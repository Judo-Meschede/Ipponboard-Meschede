'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('fs');
const os=require('os');
const path=require('path');
const {migrateLegacyCompetitionDays}=require('./legacy-competition-migration');

function fixture(t){
 const root=fs.mkdtempSync(path.join(os.tmpdir(),'ipponboard-legacy-days-'));
 t.after(()=>fs.rmSync(root,{recursive:true,force:true}));
 const folder=path.join(root,'offline-tournaments');
 fs.mkdirSync(folder);
 const master={competitionDays:[]};let saved=0;
 const add=(type,day)=>{assert.equal(type,'competitionDays');saved++;master.competitionDays.push({...day});return {...day}};
 const legacy=(id,data={})=>{
  const obj={id,name:'Test-Kampftag',teamIds:['team-a','team-b'],matCount:1,...data};
  fs.writeFileSync(path.join(folder,id+'.json'),JSON.stringify(obj));
 };
 return {root,folder,master,add,legacy,saves:()=>saved};
}
test('an old desktop-only competition day is visible in shared masterdata',t=>{
 const f=fixture(t);f.legacy('existing-day');
 const result=migrateLegacyCompetitionDays({dataDir:f.root,getMaster:()=>f.master,saveRecord:f.add});
 assert.equal(result.imported,1);assert.equal(result.failed,0);assert.equal(f.master.competitionDays.length,1);
 assert.equal(f.master.competitionDays[0].id,'existing-day');
 assert.equal(fs.existsSync(path.join(f.folder,'existing-day.json')),false);
 assert.ok(fs.existsSync(path.join(f.root,'legacy-competition-days-imported','existing-day.json')));
});
test('migration can restart without inserting duplicate competition days',t=>{
 const f=fixture(t);f.legacy('already-exists');f.master.competitionDays.push({id:'already-exists',name:'Newer canonical version'});
 const r=migrateLegacyCompetitionDays({dataDir:f.root,getMaster:()=>f.master,saveRecord:f.add});
 assert.equal(r.imported,0);assert.equal(r.previouslyPresent,1);assert.equal(f.saves(),0);
 assert.equal(f.master.competitionDays[0].name,'Newer canonical version');
});
test('failed saves leave original files intact and retryable',t=>{
 const f=fixture(t);f.legacy('failed-day');
 const fail=()=>{throw new Error('Disk write failed')};
 const result=migrateLegacyCompetitionDays({dataDir:f.root,getMaster:()=>f.master,saveRecord:fail});
 assert.equal(result.failed,1);assert.equal(result.imported,0);
 assert.ok(fs.existsSync(path.join(f.folder,'failed-day.json')));
 const retry=migrateLegacyCompetitionDays({dataDir:f.root,getMaster:()=>f.master,saveRecord:f.add});
 assert.equal(retry.imported,1);assert.equal(f.saves(),1);
});
test('invalid legacy JSON is never deleted',t=>{
 const f=fixture(t);
 fs.writeFileSync(path.join(f.folder,'bad.json'),'{invalid');
 const result=migrateLegacyCompetitionDays({dataDir:f.root,getMaster:()=>f.master,saveRecord:f.add});
 assert.equal(result.failed,1);
 assert.ok(fs.existsSync(path.join(f.folder,'bad.json')));
});
test('parallel archived filename is preserved without overwrite',t=>{
 const f=fixture(t);f.legacy('another-day');
 const old=path.join(f.root,'legacy-competition-days-imported');
 fs.mkdirSync(old);fs.writeFileSync(path.join(old,'another-day.json'),'original backup');
 const r=migrateLegacyCompetitionDays({dataDir:f.root,getMaster:()=>f.master,saveRecord:f.add});
 assert.equal(r.imported,1);
 assert.equal(fs.readFileSync(path.join(old,'another-day.json'),'utf8'),'original backup');
 assert.ok(fs.existsSync(path.join(old,'another-day.1.json')));
});
