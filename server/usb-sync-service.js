'use strict';
const fs=require('fs');
const path=require('path');
const crypto=require('crypto');
const https=require('https');
const {merge,digest,validate,same}=require('./usb-sync-core');
const SYNC_SCHEMA='ipponboard-usb-sync-1';
const DEFAULT_URL='https://test-liga.paul-meschede.de';
const now=()=>new Date().toISOString();
const copy=value=>structuredClone(value);
function read(file){return JSON.parse(fs.readFileSync(file,'utf8'))}
function write(file,value){
 fs.mkdirSync(path.dirname(file),{recursive:true});
 const tmp=file+'.tmp-'+process.pid;
 fs.writeFileSync(tmp,JSON.stringify(value,null,2),'utf8');
 fs.renameSync(tmp,file);
}
function error(message,status){const e=new Error(message);e.status=status;return e}
function sha(value){return crypto.createHash('sha256').update(value).digest('hex')}
function validateCredentials(profile){
 return profile&&profile.mode==='full'&&profile.serverSyncAllowed===true&&
 profile.sync&&typeof profile.sync==='object'&&
 /^[0-9a-f-]{36}$/.test(profile.sync.id||'')&&
 /^[0-9a-f]{64}$/.test(profile.sync.token||'')&&
 profile.sync.server===DEFAULT_URL;
}
class Hub{
 constructor(file,getMaster,saveMerged){
  this.file=file;this.getMaster=getMaster;this.saveMerged=saveMerged;
 }
 issue(){
  return {id:crypto.randomUUID(),token:crypto.randomBytes(32).toString('hex'),server:DEFAULT_URL};
 }
 register(credential,baseline){
  validate(baseline);
  const clients=fs.existsSync(this.file)?read(this.file):{};
  if(clients[credential.id])throw error('USB-Synchronisierungs-ID bereits vorhanden',409);
  if(Object.keys(clients).length>=5000)throw error('Maximale Anzahl von USB-Geräten erreicht',503);
  clients[credential.id]={secretHash:sha(credential.token),baselineHash:digest(baseline),
   issuedAt:now(),lastSync:null};
  write(this.file,clients);
 }
 authorize(id,token){
  if(typeof id!=='string'||typeof token!=='string'||id.length>70||token.length>100)
   throw error('USB-Gerät nicht autorisiert',403);
  const clients=fs.existsSync(this.file)?read(this.file):{};
  const registered=clients[id],actual=Buffer.from(sha(token),'hex');
  if(!registered||typeof registered.secretHash!=='string'||
   !/^[0-9a-f]{64}$/.test(registered.secretHash)||
   !crypto.timingSafeEqual(Buffer.from(registered.secretHash,'hex'),actual))
   throw error('USB-Gerät nicht autorisiert',403);
  return registered;
 }
 exchange(id,token,body){
  this.authorize(id,token);
  const clients=read(this.file),registered=clients[id];
  if(!body||!body.baseline||!body.local)throw error('USB-Daten fehlen',400);
  validate(body.baseline);validate(body.local);
  const incomingHash=digest(body.baseline);
  if(registered.baselineHash!==incomingHash){
   if(registered.previousBaselineHash===incomingHash&&registered.previousResponse)
    return {...copy(registered.previousResponse),replayed:true};
   throw error('Synchronisationsbasis veraltet; keine Serveränderung vorgenommen',409);
  }
  const merged=merge(body.baseline,body.local,this.getMaster());
  if(!merged.ok)return {ok:false,conflicts:merged.conflicts,status:409};
  if(merged.applied>0)this.saveMerged(merged.merged,'USB V3 Synchronisierung ('+id+')');
  const latest=copy(this.getMaster());
  const response={ok:true,masterdata:latest,applied:merged.applied,serverRevision:latest.revision};
  // A no-op heartbeat must not rewrite the pairing registry every five seconds.
  if(merged.applied===0&&digest(latest)===registered.baselineHash)
   return response;
  clients[id]={...registered,previousBaselineHash:registered.baselineHash,
   previousResponse:response,baselineHash:digest(latest),lastSync:now()};
  write(this.file,clients);
  return response;
 }
}
function requestRemote(url,credentials,body,timeoutMs=6500){
 return new Promise((resolve,reject)=>{
  const endpoint=new URL('/api/usb-sync',url);
  if(endpoint.protocol!=='https:'||endpoint.hostname!=='test-liga.paul-meschede.de'||endpoint.port)
   return reject(error('Nicht freigegebener Synchronisierungsserver',403));
  const payload=Buffer.from(JSON.stringify(body),'utf8');
  if(payload.length>24e6)return reject(error('USB-Datenbestand zu groß für den Abgleich',413));
  const req=https.request(endpoint,{method:'POST',timeout:timeoutMs,
   headers:{'content-type':'application/json','content-length':payload.length,
    'x-usb-sync-id':credentials.id,'x-usb-sync-token':credentials.token}},res=>{
   if(res.statusCode===301||res.statusCode===302||res.statusCode===307||res.statusCode===308){
    res.resume();return reject(error('Serverumleitung verweigert',502));
   }
   let chunks=[],size=0;
   res.on('data',part=>{size+=part.length;if(size>25e6){req.destroy(error('Serverantwort zu groß',413));return}chunks.push(part)});
   res.on('end',()=>{try{
    const result=JSON.parse(Buffer.concat(chunks).toString('utf8'));
    resolve({status:res.statusCode,body:result});
   }catch(e){reject(error('Ungültige Antwort des Servers',502))}});
  });
  req.on('timeout',()=>req.destroy(error('Zeitüberschreitung',504)));
  req.on('error',reject);
  req.end(payload);
 });
}
class Client{
 constructor({dataDir,profile,transport=requestRemote}){
  this.dataDir=dataDir;this.profile=profile;this.transport=transport;
  this.masterFile=path.join(dataDir,'masterdata.json');
  this.stateFile=path.join(dataDir,'usb-sync-state.json');
  this.status={enabled:validateCredentials(profile),connected:false,lastAttempt:null,
   lastSuccess:null,conflicts:[],error:null,pending:false};
  this.inFlight=false;this.timer=null;this.debounce=null;
 }
 getStatus(){
  let pending=false,masterRevision=null;
  if(this.status.enabled){
   try{
    const state=read(this.stateFile),local=read(this.masterFile).masterdata;
    masterRevision=Number(local.revision)||0;
    pending=!same(state.baseline,local);
   }catch{pending=true}
  }
  return {...this.status,pending,masterRevision};
 }
 schedule(){
  if(!this.status.enabled||this.debounce)return;
  this.debounce=setTimeout(()=>{this.debounce=null;this.sync().catch(()=>{});},400);
 }
 start(){
  if(!this.status.enabled)return;
  this.timer=setInterval(()=>this.sync().catch(()=>{}),5000);
  this.timer.unref();
  fs.watchFile(this.masterFile,{interval:1100},(after,before)=>{
   if(after.mtimeMs!==before.mtimeMs)this.schedule();
  });
  this.schedule();
 }
 stop(){
  if(this.timer)clearInterval(this.timer);
  if(this.debounce)clearTimeout(this.debounce);
  fs.unwatchFile(this.masterFile);
 }
 async sync(){
  if(this.inFlight||!this.status.enabled)return;
  this.inFlight=true;this.status.lastAttempt=now();
  try{
   const state=read(this.stateFile),root=read(this.masterFile);
   if(state.schema!==SYNC_SCHEMA||state.clientId!==this.profile.sync.id)
    throw error('USB-Synchronisierungsbasis fehlt oder gehört zu einem anderen Gerät',409);
   const baseline=validate(state.baseline),sent=validate(root.masterdata);
   const result=await this.transport(this.profile.sync.server,this.profile.sync,
    {baseline,local:sent});
   if(result.status===409){
    this.status.conflicts=Array.isArray(result.body.conflicts)?result.body.conflicts:[];
    this.status.connected=true;
    throw error(result.body.error||'Synchronisationskonflikt: Serverdaten unverändert',409);
   }
   if(result.status!==200||!result.body||!result.body.ok)
    throw error('Synchronisierung fehlgeschlagen: HTTP '+result.status,result.status);
   const remote=validate(result.body.masterdata);
   // The desktop and local admin can modify the same USB file while the
   // HTTPS request is in progress. Preserve these newer local edits.
   const current=read(this.masterFile);
   const integration=merge(sent,validate(current.masterdata),remote);
   if(!integration.ok){
    this.status.conflicts=integration.conflicts;
    throw error('Gleichzeitige lokale Änderungen: automatischer Abgleich angehalten',409);
   }
   const updated=integration.merged;
   if(!same(current.masterdata,updated)){
    const backupDir=path.join(this.dataDir,'sync-backups');
    fs.mkdirSync(backupDir,{recursive:true});
    const stamp=new Date().toISOString().replace(/[:.]/g,'-');
    fs.copyFileSync(this.masterFile,path.join(backupDir,'masterdata-'+stamp+'.json'));
    const old=fs.readdirSync(backupDir).filter(x=>x.startsWith('masterdata-')&&x.endsWith('.json')).sort();
    for(const name of old.slice(0,Math.max(0,old.length-10)))fs.unlinkSync(path.join(backupDir,name));
    write(this.masterFile,{...current,masterdata:updated});
   }
   write(this.stateFile,{...state,baseline:remote,lastSuccess:now()});
   this.status.connected=true;this.status.lastSuccess=now();
   this.status.conflicts=[];this.status.error=null;
  }catch(e){
   this.status.error=e.message;
   if(e.status!==409)this.status.connected=false;
  }finally{this.inFlight=false}
 }
}
module.exports={Hub,Client,DEFAULT_URL,SYNC_SCHEMA,validateCredentials,requestRemote};
