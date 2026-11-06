'use strict';
const fs=require('fs');
const crypto=require('crypto');
const EXPECTED_TEMPLATE_VERSION='0.2.63-preview';
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
function createUsbZip({master,mode,selected,templatePath,syncCredentials,baselineSnapshot}){
 if(!['gift','selected','full'].includes(mode))throw new Error('Unbekannter USB-Modus');
 if(!fs.existsSync(templatePath))throw new Error('USB-Vorlage auf dem Server fehlt');
 const template=new AdmZip(templatePath);
 // Windows-created ZIPs can store backslashes instead of forward slashes.
 // AdmZip.getEntry(path) uses the raw archive entry name, so a valid file
 // may not be found despite appearing correctly in Windows Explorer.
 const templateEntries=new Map();
 for(const entry of template.getEntries()){
  if(entry.isDirectory)continue;
  const name=entry.entryName.replace(/\\/g,'/');
  if(!name||name.startsWith('/')||name.split('/').includes('..'))throw new Error('Ungültiger Dateipfad in der USB-Vorlage: '+name);
  if(templateEntries.has(name))throw new Error('Doppelter Dateipfad in der USB-Vorlage: '+name);
  templateEntries.set(name,entry);
 }
 const manifestEntry=templateEntries.get('build-manifest.json');
 if(!manifestEntry)throw new Error('USB-Vorlage veraltet: Icon-/Versionsprüfung fehlt. Neue Windows-ZIP erzeugen und hochladen.');
 let manifest;
 try { manifest=JSON.parse(manifestEntry.getData().toString('utf8')); }
 catch (_) { throw new Error('USB-Buildmanifest ungültig. Neue Windows-ZIP erforderlich.'); }
 if(manifest.templateSchema!=='ipponboard-usb-template-1'||
    manifest.version!==EXPECTED_TEMPLATE_VERSION||manifest.iconEntries!==7)
  throw new Error('USB-Vorlage ist nicht die geprüfte '+EXPECTED_TEMPLATE_VERSION+'. Neue Windows-ZIP hochladen.');
 for(const [name,expected] of [
     ['Ipponboard-Meschede.exe',manifest.launcherSha256],
     ['app/Ipponboard-Meschede.exe',manifest.scoreboardSha256]
 ]) {
  const entry=templateEntries.get(name);
  if(!entry)throw new Error('USB-Vorlage enthält nicht '+name);
  const checksum=crypto.createHash('sha256').update(entry.getData()).digest('hex');
  if(!/^[a-f0-9]{64}$/.test(expected)||checksum!==expected)
   throw new Error('USB-Vorlage fehlerhaft: '+name+' stimmt nicht mit geprüftem Build überein.');
 }
 const zip=new AdmZip();
 for(const [name,entry] of templateEntries){
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
 const data=baselineSnapshot||snapshot(master,mode,selected||{});
 if(mode==='full'){
  if(!syncCredentials||!/^[0-9a-f-]{36}$/.test(syncCredentials.id||'')||
   !/^[0-9a-f]{64}$/.test(syncCredentials.token||'')||
   syncCredentials.server!=='https://test-liga.paul-meschede.de')
   throw new Error('Variante 3 benötigt eine gültige Synchronisierungskopplung');
  zip.addFile('data/usb-sync-state.json',
   Buffer.from(JSON.stringify({
    schema:'ipponboard-usb-sync-1',clientId:syncCredentials.id,
    baseline:data,lastSuccess:null
   },null,2),'utf8'));
 }
 zip.addFile('data/masterdata.json',Buffer.from(JSON.stringify({version:'0.2.63',masterdata:data},null,2),'utf8'));
 zip.addFile('data/usb-profile.json',Buffer.from(JSON.stringify({mode,serverSyncAllowed:mode==='full',createdAt:new Date().toISOString(),...(mode==='full'?{sync:syncCredentials}:{})},null,2),'utf8'));
 return zip.toBuffer();
}
module.exports={createUsbZip,snapshot};
