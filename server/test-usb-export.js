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
const buffer = createUsbZip({master, mode:'gift', selected:{}, templatePath:path.resolve(templatePath)});
const zip = new AdmZip(buffer);
const required = [
 'Ipponboard-Meschede.exe',
 'app/Ipponboard-Meschede.exe',
 'build-manifest.json',
 'data/masterdata.json',
 'data/usb-profile.json',
 'runtime/node.exe',
 'local-server/server.js'
];
for (const filename of required)
 assert.ok(zip.getEntry(filename), 'Missing export entry: '+filename);
const profile = JSON.parse(zip.getEntry('data/usb-profile.json').getData().toString('utf8'));
const data = JSON.parse(zip.getEntry('data/masterdata.json').getData().toString('utf8'));
assert.equal(profile.mode, 'gift');
assert.equal(profile.serverSyncAllowed,false);
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
console.log('OK: Vereinsneutraler USB-Export, vollständige EXEs, globale Einstellungen und Datenfilter geprüft.');
