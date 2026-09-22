'use strict';
const http=require('http'),fs=require('fs'),path=require('path'),crypto=require('crypto');
const PORT=Number(process.env.PORT||3011),HOST=process.env.HOST||'127.0.0.1',ROOT=path.join(__dirname,'public');
const DATA_DIR=process.env.IPPONBOARD_DATA_DIR||path.join(__dirname,'data');
const STATE_FILE=process.env.IPPONBOARD_STATE_FILE||path.join(DATA_DIR,'competition-state.json');
const MASTER_FILE=process.env.IPPONBOARD_MASTER_FILE||path.join(DATA_DIR,'masterdata.json');
const APP_VERSION='0.1.6';
const modes={
 'BL-M':{title:'1. Judo Bundesliga (Männer)',weights:['-60kg','-66kg','-73kg','-81kg','-90kg','-100kg','+100kg'],rounds:2,fightSeconds:240},
 'BL-F':{title:'1. Judo Bundesliga (Frauen)',weights:['-48kg','-52kg','-57kg','-63kg','-70kg','-78kg','+78kg'],rounds:2,fightSeconds:240},
 'VMM-MU18':{title:'VMM MU18',weights:['-46kg','-50kg','-55kg','-60kg','-66kg','-73kg','+73kg'],rounds:1,fightSeconds:240},
 'VMM-FU18':{title:'VMM FU18',weights:['-44kg','-48kg','-52kg','-57kg','-63kg','+63kg'],rounds:1,fightSeconds:240}
};
const mkFight=(weight,secs)=>({weight,whiteName:'',blueName:'',white:{i:0,w:0,y:0,s:0,h:0},blue:{i:0,w:0,y:0,s:0,h:0},timeMs:secs*1000,running:false,hold:{active:false,side:null,timeMs:0},saved:false});
function newState(){const mode='BL-M',cfg=modes[mode];return {version:APP_VERSION,revision:0,mode,teams:[{id:'A',name:'Team A'},{id:'B',name:'Team B'},{id:'C',name:'Team C'}],matches:[{id:'AB',home:'A',guest:'B'},{id:'BC',home:'B',guest:'C'},{id:'CA',home:'C',guest:'A'}],matchIndex:0,fightIndex:0,fights:cfg.weights.map(w=>mkFight(w,cfg.fightSeconds)),teamScore:{A:0,B:0,C:0},lastAction:'Systemstart',updatedAt:new Date().toISOString()};}
function newMaster(){const now=new Date().toISOString();return {schema:'ipponboard-meschede-masterdata-1',revision:1,updatedAt:now,clubs:[{id:'club-ssv-meschede',name:'SSV Meschede Judo',shortName:'Meschede',country:'GER',status:'active',website:'',notes:'',logo:'/assets/branding/ssv_meschede_logo.png',updatedAt:now}],teams:[],fighters:[],competitions:[],weightClasses:[]};}
function readJson(file,fallback){try{return JSON.parse(fs.readFileSync(file,'utf8'))}catch{return fallback()}}
function atomicWrite(file,obj){fs.mkdirSync(path.dirname(file),{recursive:true});const tmp=file+'.tmp';fs.writeFileSync(tmp,JSON.stringify(obj,null,2));fs.renameSync(tmp,file)}
let state=readJson(STATE_FILE,newState); state.version=APP_VERSION;
let master=readJson(MASTER_FILE,newMaster); if(!master.schema)master=newMaster();
function saveState(){try{atomicWrite(STATE_FILE,state)}catch(e){console.error('State save failed',e)}}
function saveMaster(action){master.revision=Number(master.revision||0)+1;master.updatedAt=new Date().toISOString();master.lastAction=action;try{atomicWrite(MASTER_FILE,master)}catch(e){console.error('Masterdata save failed',e)}}
function mergeMasterData(incoming){
 if(!incoming||!Array.isArray(incoming.clubs))throw new Error('invalid masterdata');
 const names=['clubs','teams','fighters','competitions','weightClasses'];
 for(const name of names){
  const source=Array.isArray(incoming[name])?incoming[name]:[];
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
function readBody(req){return new Promise((resolve,reject)=>{let s='';req.on('data',d=>{s+=d;if(s.length>8e6){reject(new Error('body too large'));req.destroy()}});req.on('end',()=>{try{resolve(s?JSON.parse(s):{})}catch(e){reject(e)}});req.on('error',reject)})}
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
function collectionName(raw){return ({clubs:'clubs',teams:'teams',fighters:'fighters',competitions:'competitions',weightClasses:'weightClasses'})[raw]||null}
function cleanRecord(type,r){const out={...(r||{})};out.id=String(out.id||`${type.slice(0,-1)}-${crypto.randomUUID()}`);out.updatedAt=new Date().toISOString();if(type==='teams'&&!Array.isArray(out.fighterIds))out.fighterIds=[];return out}
function saveRecord(type,record){const name=collectionName(type);if(!name)throw new Error('invalid collection');const item=cleanRecord(name,record),arr=master[name];const idx=arr.findIndex(x=>x.id===item.id);if(idx>=0)arr[idx]={...arr[idx],...item};else arr.push(item);saveMaster(`${name} gespeichert`);return item}
function deleteRecord(type,id){const name=collectionName(type);if(!name)throw new Error('invalid collection');const arr=master[name],before=arr.length;master[name]=arr.filter(x=>x.id!==id);if(name==='fighters')master.teams.forEach(t=>t.fighterIds=Array.isArray(t.fighterIds)?t.fighterIds.filter(fid=>fid!==id):[]);if(name==='clubs'){master.teams=master.teams.filter(t=>t.clubId!==id);master.fighters=master.fighters.filter(f=>f.clubId!==id)}if(master[name].length!==before)saveMaster(`${name} gelöscht`);return before!==master[name].length}
function wsAccept(k){return crypto.createHash('sha1').update(k+'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest('base64')}
function frame(o){const p=Buffer.from(JSON.stringify(o));if(p.length<126)return Buffer.concat([Buffer.from([0x81,p.length]),p]);const h=Buffer.alloc(4);h[0]=0x81;h[1]=126;h.writeUInt16BE(p.length,2);return Buffer.concat([h,p])}
function broadcast(){for(const s of sockets){try{s.write(frame({type:'state',state}))}catch{}}}
const server=http.createServer(async(req,res)=>{const u=new URL(req.url,`http://${req.headers.host||'localhost'}`);
 if(u.pathname==='/api/health')return json(res,{ok:true,name:'Ipponboard-Meschede',version:APP_VERSION,revision:state.revision,masterRevision:master.revision,masterUpdatedAt:master.updatedAt});
 if(u.pathname==='/api/state'&&req.method==='GET')return json(res,{state,modes});
 if(u.pathname==='/api/masterdata'&&req.method==='GET')return json(res,{masterdata:master,modes});
 if(u.pathname==='/api/sync/snapshot'&&req.method==='GET')return json(res,{ok:true,schema:master.schema,revision:master.revision,updatedAt:master.updatedAt,masterdata:master});
 if(u.pathname==='/api/masterdata/import'&&req.method==='POST'){try{const b=await readBody(req);if(!b||!b.masterdata||!Array.isArray(b.masterdata.clubs))return json(res,{error:'invalid masterdata'},400);master={...b.masterdata,schema:'ipponboard-meschede-masterdata-1'};saveMaster('Stammdaten importiert');return json(res,{ok:true,masterdata:master})}catch(e){return json(res,{error:e.message||'bad request'},400)}}
 if(u.pathname==='/api/masterdata/merge'&&req.method==='POST'){try{const b=await readBody(req);const incoming=b&&b.masterdata?b.masterdata:b;return json(res,{ok:true,masterdata:mergeMasterData(incoming)})}catch(e){return json(res,{error:e.message||'bad request'},400)}}
 const saveMatch=u.pathname.match(/^\/api\/masterdata\/(clubs|teams|fighters|competitions|weightClasses)$/);
 if(saveMatch&&req.method==='POST'){try{const b=await readBody(req);const item=saveRecord(saveMatch[1],b);return json(res,{ok:true,item,masterdata:master})}catch(e){return json(res,{error:e.message},400)}}
 const delMatch=u.pathname.match(/^\/api\/masterdata\/(clubs|teams|fighters|competitions|weightClasses)\/([^/]+)$/);
 if(delMatch&&req.method==='DELETE'){try{return json(res,{ok:deleteRecord(delMatch[1],decodeURIComponent(delMatch[2])),masterdata:master})}catch(e){return json(res,{error:e.message},400)}}
 if(u.pathname==='/api/export'&&req.method==='GET')return json(res,{format:'ipponboard-meschede-1',exportedAt:new Date().toISOString(),state,masterdata:master});
 if(u.pathname==='/api/import'&&req.method==='POST'){try{const b=await readBody(req);if(!b||!b.state||!Array.isArray(b.state.teams))return json(res,{error:'invalid import'},400);state=b.state;touch('Lokaler Datenimport');return json(res,{ok:true,state})}catch{return json(res,{error:'bad request'},400)}}
 if(u.pathname==='/api/sync/status'&&req.method==='GET')return json(res,{online:true,configured:true,masterRevision:master.revision,updatedAt:master.updatedAt,pending:0});
 if(u.pathname==='/api/action'&&req.method==='POST'){try{const a=await readBody(req);applyAction(a);return json(res,{state})}catch{return json(res,{error:'bad request'},400)}}
 let rel;
 if(u.pathname==='/') rel=process.env.IPPONBOARD_DESKTOP?'index.html':'transfer.html';
 else if(u.pathname==='/verwaltung'||u.pathname==='/verwaltung/') rel='verwaltung.html';
 else if(u.pathname==='/display') rel='display.html';
 else rel=u.pathname.slice(1);
 rel=path.normalize(rel).replace(/^(\.\.(\/|\\|$))+/, '');const f=path.join(ROOT,rel);if(!f.startsWith(ROOT)){res.writeHead(403);return res.end('Forbidden')}sendFile(res,f)
});
server.on('upgrade',(req,sock)=>{if(req.url!=='/ws'){sock.destroy();return}const k=req.headers['sec-websocket-key'];if(!k){sock.destroy();return}sock.write('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: '+wsAccept(k)+'\r\n\r\n');sockets.add(sock);sock.write(frame({type:'state',state}));sock.on('close',()=>sockets.delete(sock));sock.on('error',()=>sockets.delete(sock))});
setInterval(()=>{const now=Date.now(),d=now-lastTick;lastTick=now;const f=currentFight();let ch=false;if(f.running&&f.timeMs>0){f.timeMs=Math.max(0,f.timeMs-d);if(!f.timeMs)f.running=false;ch=true}if(f.hold.active){f.hold.timeMs+=d;ch=true}if(ch)broadcast()},100);
fs.mkdirSync(DATA_DIR,{recursive:true});if(!fs.existsSync(MASTER_FILE))atomicWrite(MASTER_FILE,master);if(!fs.existsSync(STATE_FILE))atomicWrite(STATE_FILE,state);
server.listen(PORT,HOST,()=>{const a=server.address();const port=a&&typeof a==='object'?a.port:PORT;console.log(`Ipponboard-Meschede ${APP_VERSION} ${process.env.IPPONBOARD_DESKTOP?'Desktop':'Web'} ${HOST}:${port}`);if(process.send)process.send({type:'listening',port})});
