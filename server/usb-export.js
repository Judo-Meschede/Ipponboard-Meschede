'use strict';
const fs=require('fs');
const path=require('path');
const AdmZip=require('adm-zip');

const COLLECTIONS=['clubs','teams','fighters','competitionDays','individualTournaments','individualCategories','individualRegistrations','individualDraws','individualBouts','individualPlacements','weightClasses','tournamentModes','ruleSets'];
const cleanIds=(ids)=>new Set((Array.isArray(ids)?ids:[]).filter(x=>typeof x==='string'&&x.length<128));
const pick=(items,ids)=>Array.isArray(items)?items.filter(x=>x&&ids.has(x.id)):[];
const references=(items,field)=>new Set(items.flatMap(x=>Array.isArray(x[field])?x[field]:[]));
function snapshot(master,kind,requested){
 const base={schema:master.schema||'ipponboard-meschede-masterdata-1',revision:0,updatedAt:new Date().toISOString()};
 for(const name of COLLECTIONS)base[name]=[];
 if(kind==='gift'){
  // Global settings belong to every USB variant, even with no clubs or events.
  for(const name of ['weightClasses','tournamentModes','ruleSets'])
   base[name]=structuredClone(master[name]||[]);
  base.ageClassFightTimeDefaults=structuredClone(master.ageClassFightTimeDefaults||{});
  return base;
 }
 if(kind==='full'){
  for(const name of COLLECTIONS)base[name]=structuredClone(master[name]||[]);
  base.ageClassFightTimeDefaults=structuredClone(master.ageClassFightTimeDefaults||{});
  base.revision=master.revision;
  return base;
 }
 const dayIds=cleanIds(requested.competitionDays),individualIds=cleanIds(requested.individualTournaments);
 base.competitionDays=pick(master.competitionDays,dayIds);
 base.individualTournaments=pick(master.individualTournaments,individualIds);
 const selectedDayIds=new Set(base.competitionDays.map(x=>x.id));
 const selectedIndividualIds=new Set(base.individualTournaments.map(x=>x.id));
 const linked=(item)=>selectedIndividualIds.has(item.tournamentId)||selectedIndividualIds.has(item.individualTournamentId);
 for(const name of ['individualCategories','individualRegistrations','individualDraws','individualBouts','individualPlacements'])
  base[name]=(master[name]||[]).filter(linked);
 const teamIds=references(base.competitionDays,'teamIds');
 base.teams=pick(master.teams,teamIds);
 const clubIds=new Set([...base.teams.map(x=>x.clubId),...base.competitionDays.map(x=>x.hostClubId),...base.individualTournaments.map(x=>x.hostClubId)]);
 const fighterIds=new Set([...references(base.teams,'fighterIds'),...base.individualRegistrations.map(x=>x.fighterId)]);
 base.fighters=pick(master.fighters,fighterIds);
 for(const fighter of base.fighters)clubIds.add(fighter.clubId);
 base.clubs=pick(master.clubs,clubIds);
 base.tournamentModes=structuredClone(master.tournamentModes||[]);
 base.ruleSets=structuredClone(master.ruleSets||[]);
 base.weightClasses=structuredClone(master.weightClasses||[]);
 base.ageClassFightTimeDefaults=structuredClone(master.ageClassFightTimeDefaults||{});
 return base;
}
function createUsbZip({master,mode,selected,templatePath}){
 if(!['gift','selected','full'].includes(mode))throw new Error('Unbekannter USB-Modus');
 if(!fs.existsSync(templatePath))throw new Error('USB-Vorlage auf dem Server fehlt');
 const template=new AdmZip(templatePath);
 if(!template.getEntries().some(e=>e.entryName.replace(/\\/g,'/')==='app/Ipponboard-Meschede.exe'))throw new Error('USB-Vorlage enthaelt keine Windows-Anwendung');
 const zip=new AdmZip();
 for(const entry of template.getEntries()){
  if(entry.isDirectory)continue;
  const name=entry.entryName.replace(/\\/g,'/');
  if(!name||name.startsWith('/')||name.split('/').includes('..'))throw new Error('Ungueltiger Pfad in USB-Vorlage');
  if(/^(data\/|app\/data\/|offline-tournaments\/|app\/offline-tournaments\/)/i.test(name))continue;
  if(/(masterdata-local\.json|last-session\.json|terminal-id\.txt|Ipponboard-Meschede\.ini|Ipponboard-Meschede\.log|FightersTeam\.csv|clubs\.config|fighters\.config|tournament.*\.json|.*\.autosave)$/i.test(name))continue;
  zip.addFile(name,entry.getData());
 }
 const offlinePage=path.join(__dirname,'public','verwaltung-offline.html');
 if(mode!=='full'){
  if(!fs.existsSync(offlinePage))throw new Error('Lokale Verwaltungsseite fehlt');
  zip.addFile('verwaltung-offline.html',fs.readFileSync(offlinePage));
 }
 const required=['app/Ipponboard-Meschede.exe','app/TournamentModes.ini','runtime/node.exe','local-server/server.js','START_IPPONBOARD_MESCHEDE.cmd','Ipponboard-Meschede.exe'];
 for(const requiredFile of required)
  if(!zip.getEntry(requiredFile))
   throw new Error('USB-Vorlage unvollständig: '+requiredFile+'. Bitte neue Windows-Vorlage hochladen.');
 const data=snapshot(master,mode,selected||{});
 zip.addFile('data/masterdata.json',Buffer.from(JSON.stringify({version:'0.2.58',masterdata:data},null,2),'utf8'));
 zip.addFile('data/usb-profile.json',Buffer.from(JSON.stringify({mode,serverSyncAllowed:mode==='full',createdAt:new Date().toISOString()},null,2),'utf8'));
 return zip.toBuffer();
}
module.exports={createUsbZip};
