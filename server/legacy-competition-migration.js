'use strict';
const fs=require('fs');
const path=require('path');

// Convert the scoreboard's old private competition-day files into the shared
// local masterdata. Original files are archived only AFTER successful saving.
// Existing IDs are never overwritten, even when an old file is edited.
function migrateLegacyCompetitionDays({dataDir,getMaster,saveRecord}){
 const dir=path.join(dataDir,'offline-tournaments');
 const archive=path.join(dataDir,'legacy-competition-days-imported');
 let imported=0,previouslyPresent=0,failed=0;
 if(!fs.existsSync(dir))return {imported,previouslyPresent,failed};
 for(const fileName of fs.readdirSync(dir)){
  if(!/^[a-zA-Z0-9_.-]+\.json$/.test(fileName))continue;
  const source=path.join(dir,fileName);
  try{
   if(!fs.statSync(source).isFile())continue;
   const day=JSON.parse(fs.readFileSync(source,'utf8'));
   if(!day||typeof day!=='object'||Array.isArray(day)||
      typeof day.id!=='string'||!day.id||day.id.length>128||
      typeof day.name!=='string'||!day.name.trim()||
      !Array.isArray(day.teamIds)||new Set(day.teamIds).size<2)
    throw new Error('Datei enthält keinen gültigen Mannschafts-Kampftag');
   const found=(getMaster().competitionDays||[]).some(x=>x&&x.id===day.id);
   if(!found){
    const saved=saveRecord('competitionDays',day);
    if(!saved||saved.id!==day.id)throw new Error('Gemeinsame Datenbank hat Kampftag nicht bestätigt');
    imported++;
   }else previouslyPresent++;
   fs.mkdirSync(archive,{recursive:true});
   let dest=path.join(archive,fileName);
   for(let i=1;fs.existsSync(dest);i++)dest=path.join(archive,fileName.replace(/\.json$/,'.'+i+'.json'));
   fs.renameSync(source,dest);
  }catch(e){
   failed++;
   console.error('Alter lokaler Kampftag bleibt erhalten:',fileName,e.message);
  }
 }
 return {imported,previouslyPresent,failed};
}
module.exports={migrateLegacyCompetitionDays};
