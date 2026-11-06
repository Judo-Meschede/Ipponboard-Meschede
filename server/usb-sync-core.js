'use strict';
// Conflict-safe, ID-based three-way merge shared by the central and USB servers.
// No implicit last-writer-wins; contested changes are reported, never discarded.
const crypto=require('crypto');
const COLLECTIONS=Object.freeze([
 'clubs','teams','fighters','competitionDays','individualTournaments',
 'individualCategories','individualRegistrations','individualDraws',
 'individualBouts','individualPlacements','weightClasses','tournamentModes','ruleSets'
]);
const SCHEMA='ipponboard-meschede-masterdata-1';
const simpleObject=x=>x!==null&&typeof x==='object'&&!Array.isArray(x);
function canonical(value){
 if(Array.isArray(value))return '['+value.map(canonical).join(',')+']';
 if(simpleObject(value))return '{'+Object.keys(value).sort().map(k=>JSON.stringify(k)+':'+canonical(value[k])).join(',')+'}';
 return JSON.stringify(value);
}
function same(a,b){return canonical(a)===canonical(b)}
function digest(value){return crypto.createHash('sha256').update(canonical(value)).digest('hex')}
function validate(snapshot){
 if(!simpleObject(snapshot)||snapshot.schema!==SCHEMA)throw new Error('Ungültige USB-Stammdatenstruktur');
 for(const name of COLLECTIONS){
  const entries=snapshot[name];
  if(!Array.isArray(entries)||entries.length>100000)throw new Error('Ungültige Sammlung '+name);
  const seen=new Set();
  for(const entry of entries){
   if(!simpleObject(entry)||typeof entry.id!=='string'||!entry.id||entry.id.length>128||seen.has(entry.id))
    throw new Error('Fehlende oder doppelte Datensatz-ID in '+name);
   seen.add(entry.id);
  }
 }
 if(snapshot.ageClassFightTimeDefaults!==undefined&&!simpleObject(snapshot.ageClassFightTimeDefaults))
  throw new Error('Ungültige Kampfzeit-Standardeinstellungen');
 return snapshot;
}
function merge(base,local,remote){
 validate(base);validate(local);validate(remote);
 const merged=structuredClone(remote),conflicts=[];
 let applied=0;
 for(const name of COLLECTIONS){
  const as=new Map(base[name].map(x=>[x.id,x]));
  const ls=new Map(local[name].map(x=>[x.id,x]));
  const rs=new Map(remote[name].map(x=>[x.id,x]));
  for(const id of new Set([...as.keys(),...ls.keys()])){
   const was=as.get(id),ours=ls.get(id),theirs=rs.get(id);
   if(same(was,ours))continue;
   if(same(ours,theirs))continue; // idempotent retry after an accepted write
   if(!same(was,theirs)){
    conflicts.push({collection:name,id,reason:'Beide Seiten verändert'});
    continue;
   }
   if(ours===undefined)rs.delete(id);else rs.set(id,structuredClone(ours));
   applied++;
  }
  // Keep the central database order; append locally created records.
  merged[name]=[...rs.values()];
 }
 const baseTimes=base.ageClassFightTimeDefaults||{};
 const localTimes=local.ageClassFightTimeDefaults||{};
 const remoteTimes=remote.ageClassFightTimeDefaults||{};
 const times={...remoteTimes};
 for(const key of new Set([...Object.keys(baseTimes),...Object.keys(localTimes)])){
  if(key==='__proto__'||key==='constructor'||key==='prototype')throw new Error('Ungültiger Einstellungsname');
  const was=baseTimes[key],ours=localTimes[key],theirs=remoteTimes[key];
  if(same(was,ours)||same(ours,theirs))continue;
  if(!same(was,theirs)){
   conflicts.push({collection:'ageClassFightTimeDefaults',id:key,reason:'Beide Seiten verändert'});
   continue;
  }
  if(ours===undefined)delete times[key];else times[key]=ours;
  applied++;
 }
 merged.ageClassFightTimeDefaults=times;
 return {ok:conflicts.length===0,conflicts,merged:conflicts.length?null:merged,applied};
}
module.exports={COLLECTIONS,SCHEMA,canonical,same,digest,validate,merge};
