'use strict';
/*
 * Read-only smoke test for an uploaded Windows USB template.
 * Run from the installed test-server directory so adm-zip is available.
 * No user data are loaded, and no ZIP is written to disk.
 */
const assert = require('assert').strict;
const path = require('path');
const AdmZip = require('adm-zip');
const {createUsbZip} = require('./usb-export');
const templatePath = process.argv[2];

if (!templatePath) {
 console.error('Usage: node test-usb-export.js /path/to/usb-template.zip');
 process.exit(2);
}
const master = {
 schema:'ipponboard-meschede-masterdata-1',revision:1,
 clubs:[{id:'test-club',name:'SHOULD_NOT_LEAK'}],
 teams:[{id:'test-team',name:'SHOULD_NOT_LEAK'}],
 fighters:[{id:'test-fighter',firstName:'SHOULD_NOT_LEAK'}],
 competitionDays:[{id:'test-day',name:'SHOULD_NOT_LEAK'}],
 individualTournaments:[{id:'test-tournament',name:'SHOULD_NOT_LEAK'}],
 weightClasses:[{id:'test-weight',name:'TEST'}],
 tournamentModes:[{id:'test-mode',name:'TEST'}],
 ruleSets:[{id:'test-rules',name:'TEST'}]
};
const exportVariant = (mode, selected={}) => {
 const credential={id:'00000000-0000-4000-8000-000000000001',token:'f'.repeat(64),
  server:'https://test-liga.paul-meschede.de'};
 const buffer = createUsbZip({master, mode, selected, templatePath:path.resolve(templatePath),
  ...(mode==='full'?{syncCredentials:credential}:{})});
 const zip = new AdmZip(buffer);
 const profile = JSON.parse(zip.getEntry('data/usb-profile.json').getData().toString('utf8'));
 const data = JSON.parse(zip.getEntry('data/masterdata.json').getData().toString('utf8'));
 assert.equal(profile.mode,mode);
 assert.equal(profile.serverSyncAllowed,mode==='full');
 if(mode==='full'){
  assert.equal(profile.sync.id,credential.id);
  assert.equal(profile.sync.server,credential.server);
  assert.ok(zip.getEntry('data/usb-sync-state.json'));
  const state=JSON.parse(zip.getEntry('data/usb-sync-state.json').getData().toString('utf8'));
  assert.equal(state.clientId,credential.id);
  assert.equal(state.baseline.clubs.length,1);
 }else{
  assert.ok(!profile.sync);
  assert.ok(!zip.getEntry('data/usb-sync-state.json'));
  assert.ok(!buffer.toString('latin1').includes(credential.token));
 }
 assert.ok(zip.getEntry('build-manifest.json'));
 return {buffer,zip,profile,data};
};
const {zip,data} = exportVariant('gift');
const required = [
 'Ipponboard-Meschede.exe',
 'app/Ipponboard-Meschede.exe',
 'build-manifest.json',
 'data/masterdata.json',
 'data/usb-profile.json',
 'runtime/node.exe',
 'local-server/server.js',
 'local-server/usb-sync-core.js',
 'local-server/usb-sync-service.js',
 'local-server/legacy-competition-migration.js'
];
for (const filename of required)
 assert.ok(zip.getEntry(filename), 'Missing export entry: '+filename);
assert.equal(data.masterdata.clubs.length,0);
assert.equal(data.masterdata.teams.length,0);
assert.equal(data.masterdata.fighters.length,0);
assert.equal(data.masterdata.competitionDays.length,0);
assert.equal(data.masterdata.individualTournaments.length,0);
assert.equal(data.masterdata.weightClasses.length,1);
assert.equal(data.masterdata.tournamentModes.length,1);
assert.equal(data.masterdata.ruleSets.length,1);
assert.ok(!zip.getEntry('data/masterdata.json').getData().toString('utf8').includes('SHOULD_NOT_LEAK'),
 'Unexpected club or fighter data in neutral USB export');
const selected = exportVariant('selected', {competitionDays:['test-day']});
assert.equal(selected.data.masterdata.competitionDays.length,1);
assert.equal(selected.data.masterdata.clubs.length,0);
assert.equal(selected.data.masterdata.teams.length,0);
assert.equal(selected.data.masterdata.weightClasses.length,1);
const full = exportVariant('full');
assert.equal(full.data.masterdata.clubs.length,1);
assert.equal(full.data.masterdata.fighters.length,1);
assert.equal(full.data.masterdata.teams.length,1);
assert.equal(full.data.masterdata.competitionDays.length,1);
console.log('OK: Varianten 1+2 offline, Variante 3 online freigegeben, Stammdaten und EXEs geprüft.');
