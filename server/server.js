'use strict';
const http=require('http'),fs=require('fs'),path=require('path'),crypto=require('crypto');
const ExcelJS=require('exceljs');
const PORT=Number(process.env.PORT||3011),HOST=process.env.HOST||'127.0.0.1',ROOT=path.join(__dirname,'public');
const DATA_DIR=process.env.IPPONBOARD_DATA_DIR||path.join(__dirname,'data');
const STATE_FILE=process.env.IPPONBOARD_STATE_FILE||path.join(DATA_DIR,'competition-state.json');
const MASTER_FILE=process.env.IPPONBOARD_MASTER_FILE||path.join(DATA_DIR,'masterdata.json');
const RECOVERY_FILE=process.env.IPPONBOARD_RECOVERY_FILE||path.join(DATA_DIR,'competition-recovery.json');
const APP_VERSION='0.2.28';
const modes={
 'BL-M':{title:'1. Judo Bundesliga (Männer)',weights:['-60kg','-66kg','-73kg','-81kg','-90kg','-100kg','+100kg'],rounds:2,fightSeconds:240},
 'BL-F':{title:'1. Judo Bundesliga (Frauen)',weights:['-48kg','-52kg','-57kg','-63kg','-70kg','-78kg','+78kg'],rounds:2,fightSeconds:240},
 'VMM-MU18':{title:'VMM MU18',weights:['-46kg','-50kg','-55kg','-60kg','-66kg','-73kg','+73kg'],rounds:1,fightSeconds:240},
 'VMM-FU18':{title:'VMM FU18',weights:['-44kg','-48kg','-52kg','-57kg','-63kg','+63kg'],rounds:1,fightSeconds:240}
};
const mkFight=(weight,secs)=>({weight,whiteName:'',blueName:'',white:{i:0,w:0,y:0,s:0,h:0},blue:{i:0,w:0,y:0,s:0,h:0},timeMs:secs*1000,running:false,hold:{active:false,side:null,timeMs:0},saved:false});
function defaultRuleSets(){return [
 {id:'IJF-2025',name:'IJF 2025',status:'active',hasYuko:true,awaseteIppon:true,openEndGoldenScore:true,shidoAddsPoint:false,shidoScoreCounts:false,maxShidoCount:2,maxWazaariCount:2,osaekomiYukoSeconds:5,osaekomiWazaariSeconds:10,osaekomiIpponSeconds:20,ipponTeamPoints:10,wazaariTeamPoints:7,yukoTeamPoints:5,shidoTeamPoints:1,ipponLabel:'Ippon',wazaariLabel:'Waza-ari',yukoLabel:'Yuko',shidoLabel:'Shido',hansokumakeLabel:'Hansoku-make',notes:''},
 {id:'IJF-2018',name:'IJF 2018',status:'active',hasYuko:false,awaseteIppon:true,openEndGoldenScore:true,shidoAddsPoint:false,shidoScoreCounts:false,maxShidoCount:2,maxWazaariCount:2,osaekomiYukoSeconds:0,osaekomiWazaariSeconds:10,osaekomiIpponSeconds:20,ipponTeamPoints:10,wazaariTeamPoints:7,yukoTeamPoints:5,shidoTeamPoints:1,ipponLabel:'Ippon',wazaariLabel:'Waza-ari',yukoLabel:'Yuko',shidoLabel:'Shido',hansokumakeLabel:'Hansoku-make',notes:''},
 {id:'IJF-2017',name:'IJF 2017',status:'active',hasYuko:false,awaseteIppon:false,openEndGoldenScore:true,shidoAddsPoint:false,shidoScoreCounts:false,maxShidoCount:2,maxWazaariCount:999,maxWazaariCountUnlimited:true,osaekomiYukoSeconds:0,osaekomiWazaariSeconds:10,osaekomiIpponSeconds:20,ipponTeamPoints:10,wazaariTeamPoints:7,yukoTeamPoints:5,shidoTeamPoints:1,ipponLabel:'Ippon',wazaariLabel:'Waza-ari',yukoLabel:'Yuko',shidoLabel:'Shido',hansokumakeLabel:'Hansoku-make',notes:''},
 {id:'IJF-2017 U15',name:'IJF 2017 U15',status:'active',hasYuko:false,awaseteIppon:false,openEndGoldenScore:true,shidoAddsPoint:false,shidoScoreCounts:false,maxShidoCount:3,maxWazaariCount:999,maxWazaariCountUnlimited:true,osaekomiYukoSeconds:0,osaekomiWazaariSeconds:10,osaekomiIpponSeconds:20,ipponTeamPoints:10,wazaariTeamPoints:7,yukoTeamPoints:5,shidoTeamPoints:1,ipponLabel:'Ippon',wazaariLabel:'Waza-ari',yukoLabel:'Yuko',shidoLabel:'Shido',hansokumakeLabel:'Hansoku-make',notes:''},
 {id:'IJF-2013',name:'IJF 2013',status:'active',hasYuko:true,awaseteIppon:true,openEndGoldenScore:true,shidoAddsPoint:false,shidoScoreCounts:true,maxShidoCount:3,maxWazaariCount:2,osaekomiYukoSeconds:10,osaekomiWazaariSeconds:15,osaekomiIpponSeconds:20,ipponTeamPoints:10,wazaariTeamPoints:7,yukoTeamPoints:5,shidoTeamPoints:1,ipponLabel:'Ippon',wazaariLabel:'Waza-ari',yukoLabel:'Yuko',shidoLabel:'Shido',hansokumakeLabel:'Hansoku-make',notes:''},
 {id:'Classic',name:'Classic',status:'active',hasYuko:true,awaseteIppon:true,openEndGoldenScore:false,shidoAddsPoint:true,shidoScoreCounts:true,maxShidoCount:3,maxWazaariCount:2,osaekomiYukoSeconds:15,osaekomiWazaariSeconds:20,osaekomiIpponSeconds:25,ipponTeamPoints:10,wazaariTeamPoints:7,yukoTeamPoints:5,shidoTeamPoints:1,ipponLabel:'Ippon',wazaariLabel:'Waza-ari',yukoLabel:'Yuko',shidoLabel:'Shido',hansokumakeLabel:'Hansoku-make',notes:''}
];}
function newState(){const mode='BL-M',cfg=modes[mode];return {version:APP_VERSION,revision:0,mode,teams:[{id:'A',name:'Team A'},{id:'B',name:'Team B'},{id:'C',name:'Team C'}],matches:[{id:'AB',home:'A',guest:'B'},{id:'BC',home:'B',guest:'C'},{id:'CA',home:'C',guest:'A'}],matchIndex:0,fightIndex:0,fights:cfg.weights.map(w=>mkFight(w,cfg.fightSeconds)),teamScore:{A:0,B:0,C:0},lastAction:'Systemstart',updatedAt:new Date().toISOString()};}
function newMaster(){const now=new Date().toISOString();return {schema:'ipponboard-meschede-masterdata-1',revision:1,updatedAt:now,clubs:[{id:'club-ssv-meschede',name:'SSV Meschede Judo',shortName:'Meschede',country:'GER',status:'active',website:'',notes:'',logo:'/assets/branding/ssv_meschede_logo.png',updatedAt:now}],teams:[],fighters:[],competitionDays:[],weightClasses:[],tournamentModes:[],ruleSets:defaultRuleSets()};}
function readJson(file,fallback){try{return JSON.parse(fs.readFileSync(file,'utf8'))}catch{return fallback()}}
function atomicWrite(file,obj){fs.mkdirSync(path.dirname(file),{recursive:true});const tmp=file+'.tmp';fs.writeFileSync(tmp,JSON.stringify(obj,null,2));fs.renameSync(tmp,file)}
let state=readJson(STATE_FILE,newState); state.version=APP_VERSION;
let master=readJson(MASTER_FILE,newMaster); if(!master.schema)master=newMaster();
function newRecoveryStore(){return {schema:'ipponboard-competition-recovery-1',revision:0,updatedAt:new Date().toISOString(),entries:{}}}
let recoveryStore=readJson(RECOVERY_FILE,newRecoveryStore);
if(!recoveryStore||recoveryStore.schema!=='ipponboard-competition-recovery-1'||typeof recoveryStore.entries!=='object'||Array.isArray(recoveryStore.entries))recoveryStore=newRecoveryStore();
function ensureMasterCollections(){
 for(const name of ['clubs','teams','fighters','competitionDays','weightClasses','tournamentModes'])if(!Array.isArray(master[name]))master[name]=[];
 if(!Array.isArray(master.ruleSets))master.ruleSets=defaultRuleSets();
}
ensureMasterCollections();
function saveState(){try{atomicWrite(STATE_FILE,state)}catch(e){console.error('State save failed',e)}}
function saveMaster(action){master.revision=Number(master.revision||0)+1;master.updatedAt=new Date().toISOString();master.lastAction=action;try{atomicWrite(MASTER_FILE,master)}catch(e){console.error('Masterdata save failed',e)}}
function saveRecoveryStore(){recoveryStore.revision=Number(recoveryStore.revision||0)+1;recoveryStore.updatedAt=new Date().toISOString();try{atomicWrite(RECOVERY_FILE,recoveryStore)}catch(e){console.error('Recovery save failed',e)}}
function competitionToDay(raw){
 const matches=Array.isArray(raw&&raw.matches)?raw.matches:[];
 const teamIds=[...new Set([...(Array.isArray(raw&&raw.teamIds)?raw.teamIds:[]),...matches.flatMap(m=>[m&&m.homeTeamId,m&&m.guestTeamId])].map(String).filter(Boolean))];
 return cleanRecord('competitionDays',{...(raw||{}),id:String(raw&&raw.id||('competitionDay-'+crypto.randomUUID())),hostClubId:String(raw&&raw.hostClubId||''),tournamentModeId:String(raw&&raw.tournamentModeId||''),teamIds,matCount:Number.parseInt(raw&&raw.matCount,10)||1,legacyCompetitionId:String(raw&&raw.id||'')});
}
function migrateLegacyCompetitions(target=master){
 const legacy=Array.isArray(target.competitions)?target.competitions:[];
 if(!Array.isArray(target.competitionDays))target.competitionDays=[];
 let migrated=0;
 for(const raw of legacy){
  const converted=competitionToDay(raw);
  const idx=target.competitionDays.findIndex(x=>x&&(String(x.id||'')===converted.id||(converted.legacyCompetitionId&&String(x.legacyCompetitionId||'')===converted.legacyCompetitionId)));
  if(idx>=0)target.competitionDays[idx]={...converted,...target.competitionDays[idx]};else target.competitionDays.push(converted);
  migrated++;
 }
 delete target.competitions;
 return migrated;
}
const startupMigrated=migrateLegacyCompetitions();
if(startupMigrated)saveMaster(startupMigrated+' alte Wettkämpfe nach Kampftage migriert');
function mergeMasterData(incoming){
 if(!incoming||!Array.isArray(incoming.clubs))throw new Error('invalid masterdata');
 const normalized={...incoming};
 if(Array.isArray(normalized.competitions)&&normalized.competitions.length)normalized.competitionDays=[...(Array.isArray(normalized.competitionDays)?normalized.competitionDays:[]),...normalized.competitions.map(competitionToDay)];
 delete normalized.competitions;
 const names=['clubs','teams','fighters','competitionDays','weightClasses','tournamentModes','ruleSets'];
 for(const name of names){
  const source=Array.isArray(normalized[name])?normalized[name]:[];
  if(!Array.isArray(master[name]))master[name]=[];
  for(const raw of source){
   const item=cleanRecord(name,raw),idx=master[name].findIndex(x=>x.id===item.id);
   if(idx>=0)master[name][idx]={...master[name][idx],...item};else master[name].push(item);
  }
 }
 saveMaster('Stammdaten zusammengeführt');
 return master;
}
let lastTick=Date.now();const sockets=new Set();
const ctype=f=>({'.html':'text/html; charset=utf-8','.css':'text/css; charset=utf-8','.js':'text/javascript; charset=utf-8','.png':'image/png','.jpg':'image/jpeg','.jpeg':'image/jpeg','.svg':'image/svg+xml','.zip':'application/zip','.json':'application/json; charset=utf-8'}[path.extname(f).toLowerCase()]||'application/octet-stream');
function sendFile(res,f){fs.readFile(f,(e,d)=>{if(e){res.writeHead(404);return res.end('Not found')}res.writeHead(200,{'Content-Type':ctype(f),'Cache-Control':'no-store'});res.end(d)})}
function json(res,obj,code=200){res.writeHead(code,{'Content-Type':'application/json; charset=utf-8','Cache-Control':'no-store'});res.end(JSON.stringify(obj))}
function redirect(res,location){res.writeHead(302,{'Location':location,'Cache-Control':'no-store'});res.end()}
function readBody(req){return new Promise((resolve,reject)=>{let s='';req.on('data',d=>{s+=d;if(s.length>8e6){reject(new Error('body too large'));req.destroy()}});req.on('end',()=>{try{resolve(s?JSON.parse(s):{})}catch(e){reject(e)}});req.on('error',reject)})}
function readRawBody(req,limit=20e6){return new Promise((resolve,reject)=>{const chunks=[];let size=0;req.on('data',d=>{size+=d.length;if(size>limit){reject(new Error('file too large'));req.destroy();return}chunks.push(d)});req.on('end',()=>resolve(Buffer.concat(chunks)));req.on('error',reject)})}
function sendBuffer(res,buffer,filename){res.writeHead(200,{'Content-Type':'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet','Content-Disposition':'attachment; filename="'+filename+'"','Cache-Control':'no-store'});res.end(buffer)}
function touch(a){state.revision++;state.lastAction=a;state.updatedAt=new Date().toISOString();saveState();broadcast()}
function currentMatch(){return state.matches[state.matchIndex]}
function currentFight(){return state.fights[state.fightIndex]}
function resetMatch(){const cfg=modes[state.mode];state.fights=cfg.weights.map(w=>mkFight(w,cfg.fightSeconds));state.fightIndex=0}
function applyAction(a){const f=currentFight();switch(a.type){
case 'set-teams': if(Array.isArray(a.names)&&a.names.length===3){state.teams.forEach((t,i)=>t.name=String(a.names[i]||t.name));touch('Teams geändert')} break;
case 'set-mode': if(modes[a.mode]){state.mode=a.mode;resetMatch();touch('Wettkampfmodus geändert')} break;
case 'select-match': if(Number.isInteger(a.index)&&a.index>=0&&a.index<3){state.matchIndex=a.index;resetMatch();touch('Begegnung gewechselt')} break;
case 'select-fight': if(Number.isInteger(a.index)&&a.index>=0&&a.index<state.fights.length){state.fightIndex=a.index;touch('Kampf gewechselt')} break;
case 'set-name': if(['white','blue'].includes(a.side)){f[a.side+'Name']=String(a.value||'');touch('Kämpfer geändert')} break;
case 'score': if(['white','blue'].includes(a.side)&&['i','w','y','s','h'].includes(a.key)){const d=Number(a.delta??1);if(a.key==='i'||a.key==='h')f[a.side][a.key]=d<0?0:1;else f[a.side][a.key]=Math.max(0,Math.min(a.key==='s'?4:9,f[a.side][a.key]+(d<0?-1:1)));touch(d<0?'Eingabe widerrufen':'Wertung geändert')} break;
case 'clock': if(a.action==='toggle'){f.running=!f.running&&f.timeMs>0} else if(a.action==='reset'){f.running=false;f.timeMs=modes[state.mode].fightSeconds*1000} touch('Kampfzeit geändert'); break;
case 'hold': if(a.action==='start'&&['white','blue'].includes(a.side)){f.hold={active:true,side:a.side,timeMs:0}} else if(a.action==='stop'){f.hold.active=false} else if(a.action==='switch'&&f.hold.side){f.hold.side=f.hold.side==='white'?'blue':'white'} else if(a.action==='reset'){f.hold={active:false,side:null,timeMs:0}} touch('Haltezeit geändert'); break;
case 'reset-current': {const cfg=modes[state.mode]; state.fights[state.fightIndex]=mkFight(f.weight,cfg.fightSeconds);touch('Aktuellen Kampf zurückgesetzt')} break;
case 'next-fight': if(state.fightIndex<state.fights.length-1){state.fightIndex++;touch('Nächster Kampf')} break;
case 'prev-fight': if(state.fightIndex>0){state.fightIndex--;touch('Vorheriger Kampf')} break;
case 'team-point': if(Object.prototype.hasOwnProperty.call(state.teamScore,a.team)){state.teamScore[a.team]=Math.max(0,state.teamScore[a.team]+Number(a.delta||1));touch('Mannschaftswertung geändert')} break;
}}
function collectionName(raw){return ({clubs:'clubs',teams:'teams',fighters:'fighters',competitionDays:'competitionDays',weightClasses:'weightClasses',tournamentModes:'tournamentModes',ruleSets:'ruleSets'})[raw]||null}
function cleanRecord(type,r){
 const out={...(r||{})};
 out.id=String(out.id||`${type.slice(0,-1)}-${crypto.randomUUID()}`);
 out.updatedAt=new Date().toISOString();
 if(type==='teams'&&!Array.isArray(out.fighterIds))out.fighterIds=[];
 if(type==='competitionDays'){
  out.teamIds=Array.isArray(out.teamIds)?[...new Set(out.teamIds.map(String).filter(Boolean))]:[];
  out.hostClubId=String(out.hostClubId||'');
  out.tournamentModeId=String(out.tournamentModeId||'');
  out.status=['planned','active','closed'].includes(out.status)?out.status:'planned';
  const count=Math.max(1,Math.min(20,Number.parseInt(out.matCount,10)||((Array.isArray(out.mats)&&out.mats.length)||1)));
  const existing=Array.isArray(out.mats)?out.mats:[];
  out.matCount=count;
  out.mats=Array.from({length:count},(_,i)=>{
   const previous=existing[i]||{};
   return {id:String(previous.id||`mat-${i+1}`),name:String(previous.name||`Tatami ${i+1}`)};
  });
 }
 return out;
}
function saveRecord(type,record){const name=collectionName(type);if(!name)throw new Error('invalid collection');const item=cleanRecord(name,record),arr=master[name];const idx=arr.findIndex(x=>x.id===item.id);if(idx>=0)arr[idx]={...arr[idx],...item};else arr.push(item);saveMaster(`${name} gespeichert`);return item}
function deleteRecoveryForCompetitionDay(id){
 let changed=false;
 for(const [key,entry] of Object.entries(recoveryStore.entries||{})){
  if(entry&&String(entry.competitionDayId||'')===String(id)){delete recoveryStore.entries[key];changed=true}
 }
 if(changed)saveRecoveryStore();
}
function deleteRecordNoSave(type,id){
 const name=collectionName(type);if(!name)throw new Error('invalid collection');
 const arr=master[name],before=arr.length;master[name]=arr.filter(x=>x.id!==id);
 if(name==='fighters')master.teams.forEach(t=>t.fighterIds=Array.isArray(t.fighterIds)?t.fighterIds.filter(fid=>fid!==id):[]);
 if(name==='teams')master.competitionDays.forEach(d=>d.teamIds=Array.isArray(d.teamIds)?d.teamIds.filter(tid=>tid!==id):[]);
 if(name==='clubs'){
  master.teams=master.teams.filter(t=>t.clubId!==id);
  master.fighters=master.fighters.filter(f=>f.clubId!==id);
  const remainingTeamIds=new Set(master.teams.map(t=>t.id));
  master.competitionDays.forEach(d=>{if(d.hostClubId===id)d.hostClubId='';d.teamIds=Array.isArray(d.teamIds)?d.teamIds.filter(tid=>remainingTeamIds.has(tid)):[]});
 }
 if(name==='competitionDays')deleteRecoveryForCompetitionDay(id);
 if(name==='tournamentModes')master.competitionDays.forEach(d=>{if(d.tournamentModeId===id)d.tournamentModeId=''});
 if(name==='ruleSets')master.tournamentModes.forEach(m=>{if(m.rules===id)m.rules=''});
 return before!==master[name].length;
}
function deleteRecord(type,id){const changed=deleteRecordNoSave(type,id);if(changed)saveMaster(collectionName(type)+' gelöscht');return changed}

const XLSX_META={
 clubs:{filename:'Ipponboard_Vereine.xlsx',sheet:'Vereine',columns:[['ID','id',38],['Vereinsname','name',34],['Kurzname','shortName',22],['Land','country',12],['Status','status',14],['Webseite','website',30],['Bemerkungen','notes',40],['Logo','logo',32],['Löschen','_delete',12]]},
 teams:{filename:'Ipponboard_Mannschaften.xlsx',sheet:'Mannschaften',columns:[['ID','id',38],['Mannschaftsname','name',34],['Kurzname','shortName',22],['Verein-ID','clubId',38],['Verein','clubName',34],['Kategorie / Liga','category',28],['Saison','season',12],['Status','status',14],['Kader-IDs','fighterIds',55],['Kader Pass-Nr.','fighterPassNumbers',55],['Kader','fighterNames',55],['Bemerkungen','notes',40],['Löschen','_delete',12]]},
 fighters:{filename:'Ipponboard_Wettkaempfer.xlsx',sheet:'Wettkämpfer',columns:[['ID','id',38],['Vorname','firstName',22],['Nachname','lastName',26],['Verein-ID','clubId',38],['Verein','clubName',34],['Pass-Nr.','passNumber',20],['Lizenz-Nr.','licenseNumber',20],['Geburtsdatum','birthDate',16],['Geschlecht','gender',14],['Nationalität','nationality',20],['Gewicht','weight',12],['Status','status',14],['Bemerkungen','notes',40],['Löschen','_delete',12]]},
 competitionDays:{filename:'Ipponboard_Kampftage.xlsx',sheet:'Kampftage',columns:[['ID','id',38],['Kampftag','name',38],['Datum','date',16],['Ausrichter-ID','hostClubId',38],['Ausrichter','hostClubName',34],['Ort','location',34],['Mannschaft-IDs','teamIds',60],['Mannschaften','teamNames',60],['Wettkampfmodus-ID','tournamentModeId',38],['Wettkampfmodus','tournamentModeName',34],['Anzahl Tatami','matCount',14],['Status','status',14],['Liga','league',28],['Saison','season',12],['Kampftag-Nr.','matchDay',14],['Bemerkungen','notes',45],['Löschen','_delete',12]]},
 weightClasses:{filename:'Ipponboard_Gewichtsklassen.xlsx',sheet:'Gewichtsklassen',columns:[['ID','id',38],['Bezeichnung','name',22],['Kategorie','category',24],['Min. kg','minWeight',12],['Max. kg','maxWeight',12],['Reihenfolge','order',14],['Status','status',14],['Löschen','_delete',12]]}
};
const byId=(arr,id)=>arr.find(x=>String(x&&x.id||'')===String(id||''));
const clubLabel=id=>(byId(master.clubs,id)||{}).name||'';
const teamLabel=id=>(byId(master.teams,id)||{}).name||'';
const modeLabel=id=>(byId(master.tournamentModes,id)||{}).title||'';
const normalizeKey=v=>String(v==null?'':v).trim().toLocaleLowerCase('de-DE');
function uniqueByLabel(arr,label,field='name'){const key=normalizeKey(label);if(!key)return '';const matches=arr.filter(x=>normalizeKey(x&&x[field])===key);return matches.length===1?String(matches[0].id||''):''}
function fighterByPass(pass){const key=String(pass==null?'':pass).replace(/[\s-]+/g,'').toUpperCase();if(!key)return '';const matches=master.fighters.filter(f=>String(f.passNumber||'').replace(/[\s-]+/g,'').toUpperCase()===key);return matches.length===1?String(matches[0].id||''):''}
function listText(values){return (Array.isArray(values)?values:[]).map(String).filter(Boolean).join('; ')}
function splitList(value){return String(value==null?'':value).split(/[;\n\r]+/).map(x=>x.trim()).filter(Boolean)}
function xlsxRows(type){
 if(type==='clubs')return master.clubs.map(x=>({...x,_delete:''}));
 if(type==='teams')return master.teams.map(x=>{const ids=Array.isArray(x.fighterIds)?x.fighterIds:[];return {...x,clubName:clubLabel(x.clubId),fighterIds:listText(ids),fighterPassNumbers:listText(ids.map(id=>(byId(master.fighters,id)||{}).passNumber||'').filter(Boolean)),fighterNames:listText(ids.map(id=>{const f=byId(master.fighters,id);return f?String((f.firstName||'')+' '+(f.lastName||'')).trim():''}).filter(Boolean)),_delete:''}});
 if(type==='fighters')return master.fighters.map(x=>({...x,clubName:clubLabel(x.clubId),_delete:''}));
 if(type==='competitionDays')return master.competitionDays.map(x=>({...x,hostClubName:clubLabel(x.hostClubId),teamIds:listText(x.teamIds),teamNames:listText((x.teamIds||[]).map(teamLabel).filter(Boolean)),tournamentModeName:modeLabel(x.tournamentModeId),_delete:''}));
 if(type==='weightClasses')return master.weightClasses.map(x=>({...x,_delete:''}));
 return [];
}
async function buildXlsx(type){
 const meta=XLSX_META[type];if(!meta)throw new Error('invalid xlsx type');
 const wb=new ExcelJS.Workbook();wb.creator='Ipponboard-Meschede';wb.created=new Date();
 const ws=wb.addWorksheet(meta.sheet,{views:[{state:'frozen',ySplit:1}]});
 ws.columns=meta.columns.map(c=>({header:c[0],key:c[1],width:c[2]}));ws.addRows(xlsxRows(type));
 const header=ws.getRow(1);header.font={bold:true,color:{argb:'FFFFFFFF'}};header.fill={type:'pattern',pattern:'solid',fgColor:{argb:'FF9C1016'}};header.alignment={vertical:'middle'};header.height=24;
 ws.autoFilter={from:'A1',to:{row:1,column:meta.columns.length}};
 ws.eachRow((row,rowNo)=>{if(rowNo>1)row.alignment={vertical:'top',wrapText:true}});
 meta.columns.forEach((c,i)=>{if(c[0].indexOf('ID')>=0||c[0]==='Pass-Nr.'||c[0]==='Lizenz-Nr.')ws.getColumn(i+1).numFmt='@'});
 const info=wb.addWorksheet('Hinweise');info.getColumn(1).width=110;info.getCell('A1').value='Ipponboard-Meschede – '+meta.sheet;info.getCell('A1').font={bold:true,size:16};
 const notes=['Bestehende IDs nicht verändern. Eine leere ID bei einer neuen Zeile erzeugt beim Import automatisch eine neue ID.','Zum Löschen eines bestehenden Datensatzes in der Spalte „Löschen“ JA eintragen.','ID-Spalten sind maßgeblich. Namensspalten daneben dienen der Lesbarkeit und können bei eindeutiger Zuordnung als Hilfe verwendet werden.','Listen wie Kader oder Mannschaften werden mit Semikolon getrennt.','Beim Import werden vorhandene Datensätze mit gleicher ID aktualisiert. Nicht aufgeführte Datensätze bleiben bestehen.','Die Datei darf umsortiert und gefiltert werden. Die Überschriften in Zeile 1 dürfen nicht geändert werden.'];
 notes.forEach((v,i)=>{info.getCell(i+3,1).value=v;info.getCell(i+3,1).alignment={wrapText:true,vertical:'top'}});
 return Buffer.from(await wb.xlsx.writeBuffer());
}
function excelCellText(cell){const v=cell.value;if(v===null||v===undefined)return '';if(v instanceof Date)return v.toISOString().slice(0,10);if(typeof v==='object'){if(Object.prototype.hasOwnProperty.call(v,'result'))return String(v.result==null?'':v.result).trim();if(Array.isArray(v.richText))return v.richText.map(x=>x.text||'').join('').trim();if(v.text!==undefined)return String(v.text==null?'':v.text).trim()}return String(v).trim()}
function rowObject(ws,rowNo){const headers={};ws.getRow(1).eachCell((cell,col)=>{const h=excelCellText(cell);if(h)headers[col]=h});const out={};ws.getRow(rowNo).eachCell({includeEmpty:true},(cell,col)=>{if(headers[col])out[headers[col]]=excelCellText(cell)});return out}
function isYes(v){return ['ja','yes','x','1','true'].includes(normalizeKey(v))}
function numValue(v){const t=String(v==null?'':v).trim().replace(',','.');if(!t)return '';const n=Number(t);return Number.isFinite(n)?n:''}
function resolveClub(id,name,warnings,rowNo){if(id&&byId(master.clubs,id))return id;if(name){const resolved=uniqueByLabel(master.clubs,name);if(resolved)return resolved;warnings.push('Zeile '+rowNo+': Verein „'+name+'“ nicht eindeutig gefunden.')}return id||''}
function resolveTeams(ids,names,warnings,rowNo){const result=[];for(const id of splitList(ids))if(byId(master.teams,id)&&!result.includes(id))result.push(id);if(!result.length)for(const name of splitList(names)){const id=uniqueByLabel(master.teams,name);if(id&&!result.includes(id))result.push(id);else if(name)warnings.push('Zeile '+rowNo+': Mannschaft „'+name+'“ nicht eindeutig gefunden.')}return result}
function recordFromXlsx(type,r,warnings,rowNo){
 if(type==='clubs')return {id:r['ID'],name:r['Vereinsname'],shortName:r['Kurzname'],country:r['Land'],status:r['Status'],website:r['Webseite'],notes:r['Bemerkungen'],logo:r['Logo']};
 if(type==='teams'){const fighterIds=[...new Set(splitList(r['Kader-IDs']).filter(id=>byId(master.fighters,id)))];if(!fighterIds.length)for(const pass of splitList(r['Kader Pass-Nr.'])){const id=fighterByPass(pass);if(id&&!fighterIds.includes(id))fighterIds.push(id);else if(pass)warnings.push('Zeile '+rowNo+': Pass-Nr. „'+pass+'“ nicht eindeutig gefunden.')}return {id:r['ID'],name:r['Mannschaftsname'],shortName:r['Kurzname'],clubId:resolveClub(r['Verein-ID'],r['Verein'],warnings,rowNo),category:r['Kategorie / Liga'],season:r['Saison'],status:r['Status'],fighterIds,notes:r['Bemerkungen']}};
 if(type==='fighters')return {id:r['ID'],firstName:r['Vorname'],lastName:r['Nachname'],clubId:resolveClub(r['Verein-ID'],r['Verein'],warnings,rowNo),passNumber:r['Pass-Nr.'],licenseNumber:r['Lizenz-Nr.'],birthDate:r['Geburtsdatum'],gender:r['Geschlecht'],nationality:r['Nationalität'],weight:numValue(r['Gewicht']),status:r['Status'],notes:r['Bemerkungen']};
 if(type==='competitionDays'){let tournamentModeId=r['Wettkampfmodus-ID']||'';if(!tournamentModeId&&r['Wettkampfmodus'])tournamentModeId=uniqueByLabel(master.tournamentModes,r['Wettkampfmodus'],'title');return {id:r['ID'],name:r['Kampftag'],date:r['Datum'],hostClubId:resolveClub(r['Ausrichter-ID'],r['Ausrichter'],warnings,rowNo),location:r['Ort'],teamIds:resolveTeams(r['Mannschaft-IDs'],r['Mannschaften'],warnings,rowNo),tournamentModeId,matCount:numValue(r['Anzahl Tatami']||r['Anzahl Matten'])||1,status:r['Status'],league:r['Liga'],season:r['Saison'],matchDay:numValue(r['Kampftag-Nr.']),notes:r['Bemerkungen']}};
 if(type==='weightClasses')return {id:r['ID'],name:r['Bezeichnung'],category:r['Kategorie'],minWeight:numValue(r['Min. kg']),maxWeight:numValue(r['Max. kg']),order:numValue(r['Reihenfolge']),status:r['Status']};
 return {};
}
function requiredRecordName(type,r){if(type==='fighters')return String((r.firstName||'')+(r.lastName||'')).trim();return String(r.name||'').trim()}
async function importXlsx(type,buffer){
 const meta=XLSX_META[type];if(!meta)throw new Error('invalid xlsx type');
 const wb=new ExcelJS.Workbook();await wb.xlsx.load(buffer);const ws=wb.getWorksheet(meta.sheet)||wb.worksheets.find(x=>x.name!=='Hinweise')||wb.worksheets[0];if(!ws)throw new Error('Keine Datentabelle gefunden.');
 const actualHeaders=[];ws.getRow(1).eachCell(c=>actualHeaders.push(excelCellText(c)));const missing=meta.columns.map(c=>c[0]).filter(h=>!actualHeaders.includes(h));if(missing.length)throw new Error('Fehlende Spalten: '+missing.join(', '));
 let created=0,updated=0,deleted=0,skipped=0;const warnings=[];
 for(let rowNo=2;rowNo<=ws.rowCount;rowNo++){const raw=rowObject(ws,rowNo);if(!Object.values(raw).some(v=>String(v==null?'':v).trim()))continue;const id=String(raw['ID']||'').trim();
  if(isYes(raw['Löschen'])){if(!id){warnings.push('Zeile '+rowNo+': Löschen ohne ID ignoriert.');skipped++;continue}if(deleteRecordNoSave(type,id))deleted++;else{warnings.push('Zeile '+rowNo+': ID '+id+' zum Löschen nicht gefunden.');skipped++}continue}
  const parsed=recordFromXlsx(type,raw,warnings,rowNo);const existing=id?byId(master[type],id):null;if(!requiredRecordName(type,parsed)){warnings.push('Zeile '+rowNo+': Pflichtname fehlt.');skipped++;continue}
  const item=cleanRecord(type,{...(existing||{}),...parsed,id:id||(existing&&existing.id)||undefined});
  if(existing){const idx=master[type].findIndex(x=>x.id===existing.id);master[type][idx]=item;updated++}else{master[type].push(item);created++}
 }
 saveMaster('XLSX-Import '+meta.sheet+': '+created+' neu, '+updated+' geändert, '+deleted+' gelöscht');
 return {ok:true,type,created,updated,deleted,skipped,warnings,revision:master.revision};
}


const REGISTRATION_TEMPLATE_VERSION='ipponboard-registration-template-1';
const REGISTRATION_TEMPLATE_PASSWORD='Ipponboard-Meschede';
function registrationCellStyle(cell,{fill=null,bold=false,center=false,fontSize=11}={}){
 cell.font={name:'Aptos Narrow',size:fontSize,bold};
 cell.alignment={vertical:'middle',horizontal:center?'center':'left'};
 if(fill)cell.fill={type:'pattern',pattern:'solid',fgColor:{argb:fill}};
 cell.border={
  top:{style:'thin',color:{argb:'FF000000'}},
  left:{style:'thin',color:{argb:'FF000000'}},
  bottom:{style:'thin',color:{argb:'FF000000'}},
  right:{style:'thin',color:{argb:'FF000000'}}
 };
}
function registrationEditable(cell,fill){
 registrationCellStyle(cell,{fill});
 cell.protection={locked:false};
}
async function buildRegistrationTemplate(type){
 if(!['club','team'].includes(type))throw new Error('invalid registration template type');
 const wb=new ExcelJS.Workbook();wb.creator='Ipponboard-Meschede';wb.created=new Date();
 const isClub=type==='club';
 const ws=wb.addWorksheet(isClub?'Vereinsliste':'Mannschaftsliste',{views:[{showGridLines:false}]});
 const maxCol=isClub?8:5;
 ws.getColumn(1).width=5;
 ws.getColumn(2).width=27;
 ws.getColumn(3).width=27;
 if(isClub){
  ws.getColumn(4).width=12;ws.getColumn(5).width=14;ws.getColumn(6).width=11;ws.getColumn(7).width=11;ws.getColumn(8).width=11;
 }else{
  ws.getColumn(4).width=13;ws.getColumn(5).width=32;
 }
 ws.getRow(1).height=24;ws.getRow(2).height=10;ws.getRow(3).height=24;
 for(let row=4;row<=43;row++)ws.getRow(row).height=21;

 ws.getCell('B1').value=isClub?'Verein':'Mannschaft:';
 registrationCellStyle(ws.getCell('B1'),{bold:true});
 if(isClub)ws.mergeCells('C1:H1');else ws.mergeCells('C1:E1');
 registrationEditable(ws.getCell('C1'),'FFD9D9D9');
 for(let col=4;col<=maxCol;col++){
  const cell=ws.getRow(1).getCell(col);
  cell.protection={locked:false};
  cell.fill={type:'pattern',pattern:'solid',fgColor:{argb:'FFD9D9D9'}};
 }
 const headers=isClub?['Name','Vorname','M/W','Jahrgang','AK','GK','Kyu']:['Name','Vorname','Jahrgang','Verein'];
 headers.forEach((value,index)=>{
  const cell=ws.getRow(3).getCell(index+2);cell.value=value;registrationCellStyle(cell,{bold:true});
 });
 for(let row=4;row<=43;row++){
  const no=ws.getCell(row,1);no.value=row-3;no.numFmt='00';registrationCellStyle(no,{center:true,fontSize:9});
  const fill=row%2===0?'FFD9D9D9':'FFB7B7B7';
  for(let col=2;col<=maxCol;col++)registrationEditable(ws.getCell(row,col),fill);
 }
 for(let row=4;row<=43;row++){
  if(isClub){
   ws.getCell(row,4).dataValidation={type:'list',allowBlank:true,formulae:['"männlich,weiblich"'],showErrorMessage:true,errorTitle:'M/W',error:'Bitte männlich oder weiblich auswählen.'};
   ws.getCell(row,5).dataValidation={type:'whole',operator:'between',allowBlank:true,formulae:[1900,2100],showErrorMessage:true,errorTitle:'Jahrgang',error:'Bitte vierstelligen Jahrgang eintragen.'};
  }else{
   ws.getCell(row,4).dataValidation={type:'whole',operator:'between',allowBlank:true,formulae:[1900,2100],showErrorMessage:true,errorTitle:'Jahrgang',error:'Bitte vierstelligen Jahrgang eintragen.'};
  }
 }
 const meta=wb.addWorksheet('_Ipponboard');
 meta.state='veryHidden';
 meta.getCell('A1').value=REGISTRATION_TEMPLATE_VERSION;
 meta.getCell('A2').value=type;
 meta.getCell('A3').value='Nur graue Felder bearbeiten';
 await ws.protect(REGISTRATION_TEMPLATE_PASSWORD,{
  selectLockedCells:false,selectUnlockedCells:true,formatCells:false,formatColumns:false,formatRows:false,
  insertColumns:false,insertRows:false,deleteColumns:false,deleteRows:false,sort:false,autoFilter:false
 });
 const filename=isClub?'Ipponboard_Blanko_Vereinsliste.xlsx':'Ipponboard_Blanko_Mannschaftsliste.xlsx';
 return {buffer:Buffer.from(await wb.xlsx.writeBuffer()),filename};
}
function registrationBirthYear(value){
 const text=String(value==null?'':value).trim();
 const match=text.match(/\b(19|20)\d{2}\b/);
 if(!match)return '';
 const year=Number(match[0]);
 return year>=1900&&year<=2100?String(year):'';
}
function fighterBirthYear(fighter){
 const direct=registrationBirthYear(fighter&&fighter.birthYear);
 if(direct)return direct;
 return registrationBirthYear(fighter&&fighter.birthDate);
}
function registrationGender(value){
 const key=normalizeKey(value);
 if(['m','männlich','maennlich','male'].includes(key))return 'm';
 if(['w','weiblich','female'].includes(key))return 'w';
 return '';
}
function registrationFindClub(name){
 const key=normalizeKey(name);if(!key)return [];
 return master.clubs.filter(c=>normalizeKey(c&&c.name)===key);
}
function registrationEnsureClub(name,warnings,context){
 const clean=String(name||'').trim();if(!clean)return null;
 const matches=registrationFindClub(clean);
 if(matches.length===1)return matches[0];
 if(matches.length>1){warnings.push(context+': Verein „'+clean+'“ ist mehrfach vorhanden.');return null}
 const club=cleanRecord('clubs',{name:clean,shortName:clean,country:'GER',status:'active',notes:'Über Meldeliste angelegt'});
 master.clubs.push(club);return club;
}
function registrationFighterMatches(firstName,lastName,birthYear,clubId){
 const first=normalizeKey(firstName),last=normalizeKey(lastName),year=String(birthYear||'');
 return master.fighters.filter(f=>
  normalizeKey(f&&f.firstName)===first&&normalizeKey(f&&f.lastName)===last&&
  fighterBirthYear(f)===year&&String(f&&f.clubId||'')===String(clubId||''));
}
function registrationUpsertFighter({firstName,lastName,birthYear,clubId,gender='',ageClass='',weightClass='',kyu=''},warnings,rowNo){
 const matches=registrationFighterMatches(firstName,lastName,birthYear,clubId);
 if(matches.length>1){warnings.push('Zeile '+rowNo+': '+firstName+' '+lastName+' ist nicht eindeutig zuzuordnen.');return {fighter:null,created:false,updated:false}}
 if(matches.length===1){
  const current=matches[0],idx=master.fighters.findIndex(f=>f.id===current.id);
  const updated=cleanRecord('fighters',{...current,firstName,lastName,birthYear,clubId,
   gender:gender||current.gender||'',ageClass:ageClass||current.ageClass||'',weightClass:weightClass||current.weightClass||'',kyu:kyu||current.kyu||'',status:current.status||'active'});
  master.fighters[idx]=updated;return {fighter:updated,created:false,updated:true};
 }
 const fighter=cleanRecord('fighters',{firstName,lastName,birthYear,clubId,gender,ageClass,weightClass,kyu,status:'active',notes:'Über Meldeliste angelegt'});
 master.fighters.push(fighter);return {fighter,created:true,updated:false};
}
function registrationWorksheet(wb,type){
 const expected=type==='club'?'Vereinsliste':'Mannschaftsliste';
 return wb.getWorksheet(expected)||wb.worksheets.find(ws=>ws.state!=='veryHidden'&&!ws.name.startsWith('_'))||wb.worksheets[0];
}
function validateRegistrationHeaders(ws,type){
 const expected=type==='club'?['Name','Vorname','M/W','Jahrgang','AK','GK','Kyu']:['Name','Vorname','Jahrgang','Verein'];
 const actual=expected.map((_,i)=>excelCellText(ws.getCell(3,i+2)));
 const missing=expected.filter((name,i)=>normalizeKey(actual[i])!==normalizeKey(name));
 if(missing.length)throw new Error('Vorlage nicht erkannt. Erwartete Überschriften: '+expected.join(', '));
}
async function importRegistrationTemplate(type,buffer){
 if(!['club','team'].includes(type))throw new Error('invalid registration template type');
 const wb=new ExcelJS.Workbook();await wb.xlsx.load(buffer);
 const meta=wb.getWorksheet('_Ipponboard');
 if(meta){
  const marker=excelCellText(meta.getCell('A1')),fileType=excelCellText(meta.getCell('A2'));
  if(marker&&marker!==REGISTRATION_TEMPLATE_VERSION)throw new Error('Unbekannte Meldelisten-Version.');
  if(fileType&&fileType!==type)throw new Error('Falscher Meldelist-Typ.');
 }
 const ws=registrationWorksheet(wb,type);if(!ws)throw new Error('Keine Tabelle gefunden.');
 validateRegistrationHeaders(ws,type);
 const warnings=[];let fightersCreated=0,fightersUpdated=0,skipped=0,clubsCreatedBefore=master.clubs.length;
 if(type==='club'){
  const clubName=excelCellText(ws.getCell('C1')).trim();
  if(!clubName)throw new Error('Verein fehlt im grauen Feld oben.');
  const club=registrationEnsureClub(clubName,warnings,'Vereinsliste');
  if(!club)throw new Error('Verein konnte nicht eindeutig zugeordnet werden.');
  for(let rowNo=4;rowNo<=ws.rowCount;rowNo++){
   const lastName=excelCellText(ws.getCell(rowNo,2)).trim(),firstName=excelCellText(ws.getCell(rowNo,3)).trim();
   const gender=registrationGender(excelCellText(ws.getCell(rowNo,4)));
   const birthYear=registrationBirthYear(excelCellText(ws.getCell(rowNo,5)));
   const ageClass=excelCellText(ws.getCell(rowNo,6)).trim(),weightClass=excelCellText(ws.getCell(rowNo,7)).trim(),kyu=excelCellText(ws.getCell(rowNo,8)).trim();
   if(!lastName&&!firstName&&!birthYear&&!gender&&!ageClass&&!weightClass&&!kyu)continue;
   if(!lastName||!firstName||!birthYear){warnings.push('Zeile '+rowNo+': Name, Vorname und Jahrgang sind Pflicht.');skipped++;continue}
   const result=registrationUpsertFighter({firstName,lastName,birthYear,clubId:club.id,gender,ageClass,weightClass,kyu},warnings,rowNo);
   if(result.created)fightersCreated++;else if(result.updated)fightersUpdated++;else skipped++;
  }
  saveMaster('Vereins-Meldeliste importiert: '+clubName);
  return {ok:true,type,clubName,clubsCreated:master.clubs.length-clubsCreatedBefore,fightersCreated,fightersUpdated,skipped,warnings,revision:master.revision};
 }
 const teamName=excelCellText(ws.getCell('C1')).trim();
 if(!teamName)throw new Error('Mannschaft fehlt im grauen Feld oben.');
 const teamMatches=master.teams.filter(t=>normalizeKey(t&&t.name)===normalizeKey(teamName));
 if(teamMatches.length>1)throw new Error('Mannschaft „'+teamName+'“ ist mehrfach vorhanden.');
 const rosterIds=[],rosterClubIds=[];
 for(let rowNo=4;rowNo<=ws.rowCount;rowNo++){
  const lastName=excelCellText(ws.getCell(rowNo,2)).trim(),firstName=excelCellText(ws.getCell(rowNo,3)).trim();
  const birthYear=registrationBirthYear(excelCellText(ws.getCell(rowNo,4))),clubName=excelCellText(ws.getCell(rowNo,5)).trim();
  if(!lastName&&!firstName&&!birthYear&&!clubName)continue;
  if(!lastName||!firstName||!birthYear||!clubName){warnings.push('Zeile '+rowNo+': Name, Vorname, Jahrgang und Verein sind Pflicht.');skipped++;continue}
  const club=registrationEnsureClub(clubName,warnings,'Zeile '+rowNo);
  if(!club){skipped++;continue}
  const result=registrationUpsertFighter({firstName,lastName,birthYear,clubId:club.id},warnings,rowNo);
  if(result.created)fightersCreated++;else if(result.updated)fightersUpdated++;else{skipped++;continue}
  if(result.fighter&&!rosterIds.includes(result.fighter.id)){rosterIds.push(result.fighter.id);rosterClubIds.push(club.id)}
 }
 if(!rosterIds.length)throw new Error('Keine gültigen Wettkämpfer in der Mannschaftsliste gefunden.');
 const uniqueClubIds=[...new Set(rosterClubIds)];
 let teamCreated=false,teamUpdated=false,team;
 if(teamMatches.length===1){
  team=cleanRecord('teams',{...teamMatches[0],fighterIds:rosterIds,clubId:uniqueClubIds.length===1?uniqueClubIds[0]:(teamMatches[0].clubId||''),status:teamMatches[0].status||'active'});
  master.teams[master.teams.findIndex(t=>t.id===team.id)]=team;teamUpdated=true;
 }else{
  team=cleanRecord('teams',{name:teamName,shortName:teamName,clubId:uniqueClubIds.length===1?uniqueClubIds[0]:'',fighterIds:rosterIds,status:'active',notes:'Über Mannschaftsliste angelegt'});
  master.teams.push(team);teamCreated=true;
 }
 saveMaster('Mannschafts-Meldeliste importiert: '+teamName);
 return {ok:true,type,teamName,clubsCreated:master.clubs.length-clubsCreatedBefore,fightersCreated,fightersUpdated,skipped,teamCreated,teamUpdated,rosterCount:rosterIds.length,warnings,revision:master.revision};
}

function recoveryKey(dayId,matId){return encodeURIComponent(String(dayId))+'::'+encodeURIComponent(String(matId))}
function sanitizeRecoverySnapshot(raw){
 if(!raw||typeof raw!=='object'||Array.isArray(raw))throw new Error('invalid recovery snapshot');
 const snapshot=JSON.parse(JSON.stringify(raw));
 const rounds=Array.isArray(snapshot.Rounds)?snapshot.Rounds:[];
 for(const round of rounds){
  if(!Array.isArray(round))continue;
  for(const fight of round){
   if(!fight||typeof fight!=='object'||fight.IsSaved===true)continue;
   fight.SecondsElapsed=0;
   fight.IsGoldenScore=false;
   for(const key of ['FirstFighter','SecondFighter']){
    if(!fight[key]||typeof fight[key]!=='object')fight[key]={};
    for(const score of ['Ippon','Wazaari','Yuko','Shido','Hansokumake'])fight[key][score]=0;
   }
  }
 }
 snapshot.Rounds=rounds;
 return snapshot;
}
function recoveryEventFight(snapshot,roundIndex,fightIndex){
 const rounds=Array.isArray(snapshot&&snapshot.Rounds)?snapshot.Rounds:[];
 const round=Number.isInteger(roundIndex)&&roundIndex>=0&&roundIndex<rounds.length&&Array.isArray(rounds[roundIndex])?rounds[roundIndex]:null;
 if(!round||!Number.isInteger(fightIndex)||fightIndex<0||fightIndex>=round.length)return null;
 const fight=round[fightIndex];
 return fight&&typeof fight==='object'&&fight.IsSaved===true?fight:null;
}
function wsAccept(k){return crypto.createHash('sha1').update(k+'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest('base64')}
function frame(o){const p=Buffer.from(JSON.stringify(o));if(p.length<126)return Buffer.concat([Buffer.from([0x81,p.length]),p]);const h=Buffer.alloc(4);h[0]=0x81;h[1]=126;h.writeUInt16BE(p.length,2);return Buffer.concat([h,p])}
function broadcast(){for(const s of sockets){try{s.write(frame({type:'state',state}))}catch{}}}
const server=http.createServer(async(req,res)=>{const u=new URL(req.url,`http://${req.headers.host||'localhost'}`);
 if(u.pathname==='/api/health')return json(res,{ok:true,name:'Ipponboard-Meschede',version:APP_VERSION,revision:state.revision,masterRevision:master.revision,masterUpdatedAt:master.updatedAt});
 if(u.pathname==='/api/state'&&req.method==='GET')return json(res,{state,modes});
 if(u.pathname==='/api/masterdata'&&req.method==='GET')return json(res,{masterdata:master,modes});
 if(u.pathname==='/api/sync/snapshot'&&req.method==='GET')return json(res,{ok:true,schema:master.schema,revision:master.revision,updatedAt:master.updatedAt,masterdata:master});
 if(u.pathname==='/api/masterdata/import'&&req.method==='POST'){try{const b=await readBody(req);if(!b||!b.masterdata||!Array.isArray(b.masterdata.clubs))return json(res,{error:'invalid masterdata'},400);master={...b.masterdata,schema:'ipponboard-meschede-masterdata-1'};ensureMasterCollections();migrateLegacyCompetitions();master.competitionDays=master.competitionDays.map(x=>cleanRecord('competitionDays',x));saveMaster('Stammdaten importiert');return json(res,{ok:true,masterdata:master})}catch(e){return json(res,{error:e.message||'bad request'},400)}}
 if(u.pathname==='/api/masterdata/merge'&&req.method==='POST'){try{const b=await readBody(req);const incoming=b&&b.masterdata?b.masterdata:b;return json(res,{ok:true,masterdata:mergeMasterData(incoming)})}catch(e){return json(res,{error:e.message||'bad request'},400)}}
 if(u.pathname==='/api/masterdata/tournamentModes'&&req.method==='PUT'){try{
  const b=await readBody(req),items=Array.isArray(b&&b.tournamentModes)?b.tournamentModes:null;
  if(!items)return json(res,{error:'invalid tournament modes'},400);
  master.tournamentModes=items.map(raw=>cleanRecord('tournamentModes',raw));
  saveMaster('Wettkampfmodi gespeichert');
  return json(res,{ok:true,tournamentModes:master.tournamentModes,revision:master.revision,updatedAt:master.updatedAt});
 }catch(e){return json(res,{error:e.message||'bad request'},400)}}
 const recoveryMatch=u.pathname.match(/^\/api\/competition-recovery\/([^/]+)\/([^/]+)$/);
 if(recoveryMatch){
  const competitionDayId=decodeURIComponent(recoveryMatch[1]),matId=decodeURIComponent(recoveryMatch[2]),key=recoveryKey(competitionDayId,matId);
  if(req.method==='GET'){
   const entry=recoveryStore.entries[key];
   return entry?json(res,{ok:true,recovery:entry}):json(res,{error:'not found'},404);
  }
  if(req.method==='PUT'){
   try{
    if(!master.competitionDays.some(d=>String(d.id||'')===competitionDayId))return json(res,{error:'competition day not found'},404);
    const body=await readBody(req);
    const snapshot=sanitizeRecoverySnapshot(body&&body.snapshot);
    const eventRound=Number.parseInt(body&&body.eventRound,10),eventFight=Number.parseInt(body&&body.eventFight,10);
    const eventData=recoveryEventFight(snapshot,eventRound,eventFight);
    if(!eventData)return json(res,{error:'event fight is not completed'},400);
    const now=new Date().toISOString(),existing=recoveryStore.entries[key]||{};
    const fights={...(existing.fights||{})},fightKey=eventRound+':'+eventFight,oldFight=fights[fightKey]||{};
    fights[fightKey]={round:eventRound,fight:eventFight,revision:Number(oldFight.revision||0)+1,updatedAt:now,terminalId:String(body.terminalId||''),reason:String(body.reason||'completed'),data:eventData};
    const entry={
     competitionDayId,matId,revision:Number(existing.revision||0)+1,updatedAt:now,
     terminalId:String(body.terminalId||''),lastReason:String(body.reason||'completed'),
     lastEvent:{round:eventRound,fight:eventFight},fights,snapshot
    };
    recoveryStore.entries[key]=entry;saveRecoveryStore();
    return json(res,{ok:true,recovery:entry});
   }catch(e){return json(res,{error:e.message||'bad recovery request'},400)}
  }
  return json(res,{error:'method not allowed'},405);
 }
 const saveMatch=u.pathname.match(/^\/api\/masterdata\/(clubs|teams|fighters|competitionDays|weightClasses|tournamentModes|ruleSets)$/);
 if(saveMatch&&req.method==='POST'){try{const b=await readBody(req);const item=saveRecord(saveMatch[1],b);return json(res,{ok:true,item,masterdata:master})}catch(e){return json(res,{error:e.message},400)}}
 const delMatch=u.pathname.match(/^\/api\/masterdata\/(clubs|teams|fighters|competitionDays|weightClasses|tournamentModes|ruleSets)\/([^/]+)$/);
 if(delMatch&&req.method==='DELETE'){try{return json(res,{ok:deleteRecord(delMatch[1],decodeURIComponent(delMatch[2])),masterdata:master})}catch(e){return json(res,{error:e.message},400)}}
 const registrationMatch=u.pathname.match(/^\/api\/registration-template\/(club|team)$/);
 if(registrationMatch&&req.method==='GET'){try{const result=await buildRegistrationTemplate(registrationMatch[1]);return sendBuffer(res,result.buffer,result.filename)}catch(e){return json(res,{error:e.message||'Meldeliste konnte nicht erstellt werden'},500)}}
 if(registrationMatch&&req.method==='POST'){try{const buffer=await readRawBody(req);if(!buffer.length)return json(res,{error:'Leere XLSX-Datei'},400);return json(res,await importRegistrationTemplate(registrationMatch[1],buffer))}catch(e){return json(res,{error:e.message||'Meldeliste konnte nicht importiert werden'},400)}}
 const xlsxMatch=u.pathname.match(/^\/api\/xlsx\/(clubs|teams|fighters|competitionDays|weightClasses)$/);
 if(xlsxMatch&&req.method==='GET'){try{const type=xlsxMatch[1],buffer=await buildXlsx(type);return sendBuffer(res,buffer,XLSX_META[type].filename)}catch(e){return json(res,{error:e.message||'XLSX export failed'},500)}}
 if(xlsxMatch&&req.method==='POST'){try{const buffer=await readRawBody(req);if(!buffer.length)return json(res,{error:'Leere XLSX-Datei'},400);return json(res,await importXlsx(xlsxMatch[1],buffer))}catch(e){return json(res,{error:e.message||'XLSX import failed'},400)}}
 if(u.pathname==='/api/export'&&req.method==='GET')return json(res,{format:'ipponboard-meschede-1',exportedAt:new Date().toISOString(),state,masterdata:master,recovery:recoveryStore});
 if(u.pathname==='/api/import'&&req.method==='POST'){try{const b=await readBody(req);if(!b||!b.state||!Array.isArray(b.state.teams))return json(res,{error:'invalid import'},400);state=b.state;if(b.recovery&&b.recovery.schema==='ipponboard-competition-recovery-1'&&b.recovery.entries&&typeof b.recovery.entries==='object'){recoveryStore=b.recovery;saveRecoveryStore()}touch('Lokaler Datenimport');return json(res,{ok:true,state,recovery:recoveryStore})}catch{return json(res,{error:'bad request'},400)}}
 if(u.pathname==='/api/sync/status'&&req.method==='GET')return json(res,{online:true,configured:true,masterRevision:master.revision,updatedAt:master.updatedAt,pending:0});
 if(u.pathname==='/api/action'&&req.method==='POST'){try{const a=await readBody(req);applyAction(a);return json(res,{state})}catch{return json(res,{error:'bad request'},400)}}
 let rel;
 const desktopMode=Boolean(process.env.IPPONBOARD_DESKTOP);
 if(!desktopMode&&(u.pathname==='/'||u.pathname==='/index.html'||u.pathname==='/transfer.html'))return redirect(res,'/verwaltung');
 if(u.pathname==='/') rel='index.html';
 else if(u.pathname==='/verwaltung'||u.pathname==='/verwaltung/') rel='verwaltung.html';
 else if(u.pathname==='/display') rel='display.html';
 else rel=u.pathname.slice(1);
 rel=path.normalize(rel).replace(/^(\.\.(\/|\\|$))+/, '');const f=path.join(ROOT,rel);if(!f.startsWith(ROOT)){res.writeHead(403);return res.end('Forbidden')}sendFile(res,f)
});
server.on('upgrade',(req,sock)=>{if(req.url!=='/ws'){sock.destroy();return}const k=req.headers['sec-websocket-key'];if(!k){sock.destroy();return}sock.write('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: '+wsAccept(k)+'\r\n\r\n');sockets.add(sock);sock.write(frame({type:'state',state}));sock.on('close',()=>sockets.delete(sock));sock.on('error',()=>sockets.delete(sock))});
setInterval(()=>{const now=Date.now(),d=now-lastTick;lastTick=now;const f=currentFight();let ch=false;if(f.running&&f.timeMs>0){f.timeMs=Math.max(0,f.timeMs-d);if(!f.timeMs)f.running=false;ch=true}if(f.hold.active){f.hold.timeMs+=d;ch=true}if(ch)broadcast()},100);
fs.mkdirSync(DATA_DIR,{recursive:true});if(!fs.existsSync(MASTER_FILE))atomicWrite(MASTER_FILE,master);if(!fs.existsSync(STATE_FILE))atomicWrite(STATE_FILE,state);if(!fs.existsSync(RECOVERY_FILE))atomicWrite(RECOVERY_FILE,recoveryStore);
server.listen(PORT,HOST,()=>{const a=server.address();const port=a&&typeof a==='object'?a.port:PORT;console.log(`Ipponboard-Meschede ${APP_VERSION} ${process.env.IPPONBOARD_DESKTOP?'Desktop':'Web'} ${HOST}:${port}`);if(process.send)process.send({type:'listening',port})});
