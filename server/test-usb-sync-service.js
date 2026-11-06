'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('fs');
const os=require('os');
const path=require('path');
const {Hub,Client,DEFAULT_URL,SYNC_SCHEMA}=require('./usb-sync-service');
const {COLLECTIONS,SCHEMA,digest,same}=require('./usb-sync-core');
function master(){
 const x={schema:SCHEMA,revision:1,updatedAt:'2026-11-06T00:00:00Z',
 ageClassFightTimeDefaults:{U15:180}};
 for(const name of COLLECTIONS)x[name]=[];
 x.clubs=[{id:'club-a',name:'First'}];
 x.fighters=[{id:'f-a',firstName:'Alice',clubId:'club-a'}];
 return x;
}
const clone=x=>structuredClone(x);
function setup(t){
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'ipponboard-v3-sync-'));
 t.after(()=>fs.rmSync(dir,{recursive:true,force:true}));
 let remote=master(),writes=0;
 const hub=new Hub(path.join(dir,'hub','clients.json'),()=>remote,(merged)=>{
  remote=merged;remote.revision++;remote.updatedAt='2026-11-06T10:00:00Z';writes++;
 });
 const pairing=hub.issue();hub.register(pairing,remote);
 const dataDir=path.join(dir,'usb');fs.mkdirSync(dataDir);
 fs.writeFileSync(path.join(dataDir,'masterdata.json'),JSON.stringify({version:'test',masterdata:remote}));
 fs.writeFileSync(path.join(dataDir,'usb-sync-state.json'),JSON.stringify({
  schema:SYNC_SCHEMA,clientId:pairing.id,baseline:remote
 }));
 const profile={mode:'full',serverSyncAllowed:true,sync:pairing};
 let online=true;
 const transport=async(_url,credentials,body)=>{
  if(!online)throw Error('offline');
  try {
   const out=hub.exchange(credentials.id,credentials.token,body);
   return {status:out.ok?200:out.status,body:out};
  }catch(e){return {status:e.status||500,body:{error:e.message}}}
 };
 const client=new Client({dataDir,profile,transport});
 const local=()=>JSON.parse(fs.readFileSync(client.masterFile,'utf8')).masterdata;
 const edit=fn=>{const root=JSON.parse(fs.readFileSync(client.masterFile,'utf8'));fn(root.masterdata);fs.writeFileSync(client.masterFile,JSON.stringify(root))};
 return {hub,client,pairing,local,edit,remote:()=>remote,updateRemote:fn=>fn(remote),
  setOnline:v=>online=v,writes:()=>writes,dataDir};
}
test('unique pairing stores only the token hash',t=>{
 const s=setup(t);const file=fs.readFileSync(s.hub.file,'utf8');
 assert.ok(!file.includes(s.pairing.token));
 assert.throws(()=>s.hub.exchange(s.pairing.id,'a'.repeat(64),{baseline:master(),local:master()}),/nicht autorisiert/);
});
test('remote change is downloaded with no accidental writes to server',async t=>{
 const s=setup(t);s.updateRemote(r=>r.clubs.push({id:'new',name:'Server'}));
 await s.client.sync();
 assert.equal(s.local().clubs.length,2);
 assert.equal(s.writes(),0);
 assert.equal(s.client.getStatus().pending,false);
 assert.ok(s.client.getStatus().lastSuccess);
});
test('local edits synchronize immediately and then become idempotent',async t=>{
 const s=setup(t);s.edit(r=>r.clubs[0].name='USB');
 await s.client.sync();
 assert.equal(s.remote().clubs[0].name,'USB');
 assert.equal(s.writes(),1);
 await s.client.sync();
 assert.equal(s.writes(),1);
 assert.equal(s.client.getStatus().pending,false);
});
test('offline edits queue and sync on reconnection',async t=>{
 const s=setup(t);
 s.setOnline(false);s.edit(r=>r.clubs[0].name='Offline');
 await s.client.sync();
 assert.equal(s.remote().clubs[0].name,'First');
 assert.equal(s.client.getStatus().pending,true);
 s.setOnline(true);await s.client.sync();
 assert.equal(s.remote().clubs[0].name,'Offline');
 assert.equal(s.client.getStatus().pending,false);
});
test('conflict never overwrites central data or local data',async t=>{
 const s=setup(t);
 s.edit(r=>r.clubs[0].name='Local change');
 s.updateRemote(r=>r.clubs[0].name='Remote change');
 await s.client.sync();
 assert.equal(s.remote().clubs[0].name,'Remote change');
 assert.equal(s.local().clubs[0].name,'Local change');
 assert.equal(s.client.getStatus().conflicts.length,1);
 assert.equal(s.writes(),0);
});
test('retry after lost response replays without duplicate writes',async t=>{
 const s=setup(t);
 s.edit(r=>r.fighters.push({id:'new-f',firstName:'Bob'}));
 const first={baseline:JSON.parse(fs.readFileSync(s.client.stateFile,'utf8')).baseline,local:s.local()};
 const out=s.hub.exchange(s.pairing.id,s.pairing.token,first);
 assert.equal(out.ok,true);assert.equal(s.writes(),1);
 await s.client.sync();
 assert.equal(s.writes(),1);
 assert.equal(s.local().fighters.length,2);
});
test('concurrent local edits during remote request are preserved',async t=>{
 const s=setup(t);
 const normal=s.client.transport;
 s.client.transport=async(url,cred,body)=>{
  s.edit(r=>r.fighters[0].firstName='Updated during request');
  return normal(url,cred,body);
 };
 await s.client.sync();
 assert.equal(s.local().fighters[0].firstName,'Updated during request');
 assert.equal(s.client.getStatus().pending,true);
 s.client.transport=normal;
 await s.client.sync();
 assert.equal(s.remote().fighters[0].firstName,'Updated during request');
});
test('V1 and V2 never issue network requests',async t=>{
 const s=setup(t);
 for(const mode of ['gift','selected']){
  let calls=0;
  const client=new Client({dataDir:s.dataDir,
   profile:{mode,serverSyncAllowed:false,sync:s.pairing},
   transport:async()=>{calls++;throw Error('Unexpected network call')}});
  client.start();await client.sync();client.stop();
  assert.equal(calls,0);
  assert.equal(client.getStatus().enabled,false);
 }
});
