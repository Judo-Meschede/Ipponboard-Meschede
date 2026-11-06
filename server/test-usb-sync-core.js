'use strict';
const assert=require('node:assert/strict');
const test=require('node:test');
const {COLLECTIONS,SCHEMA,merge,digest,same}=require('./usb-sync-core');
function base(){
 const o={schema:SCHEMA,revision:1,updatedAt:'2026-01-01T00:00:00Z',
  ageClassFightTimeDefaults:{U15:180}};
 for(const name of COLLECTIONS)o[name]=[];
 o.clubs=[{id:'a',name:'A'},{id:'b',name:'B'}];
 o.fighters=[{id:'f',firstName:'First',clubId:'a'}];
 return o;
}
const clone=x=>structuredClone(x);
test('same data produces no writes',()=>{
 const x=base(),r=merge(x,x,x);
 assert.equal(r.ok,true);assert.equal(r.applied,0);assert.ok(same(r.merged,x));
});
test('local changes and separate remote changes are both retained',()=>{
 const x=base(),l=clone(x),r=clone(x);
 l.clubs[0].name='Updated A';r.clubs[1].name='Updated B';r.revision=17;
 const out=merge(x,l,r);
 assert.equal(out.ok,true);assert.equal(out.applied,1);
 assert.equal(out.merged.clubs[0].name,'Updated A');
 assert.equal(out.merged.clubs[1].name,'Updated B');
 assert.equal(out.merged.revision,17);
});
test('conflicting edits are rejected as a unit',()=>{
 const x=base(),l=clone(x),r=clone(x);
 l.clubs[0].name='USB';r.clubs[0].name='Server';
 l.clubs[1].name='USB B';
 const out=merge(x,l,r);
 assert.equal(out.ok,false);
 assert.equal(out.merged,null);
 assert.deepEqual(out.conflicts.map(y=>y.id),['a']);
});
test('idempotent retries cannot duplicate records',()=>{
 const x=base(),l=clone(x),r=clone(x);
 l.clubs.push({id:'new',name:'New'});
 r.clubs.push({id:'new',name:'New'});
 assert.equal(merge(x,l,r).applied,0);
 assert.equal(merge(x,l,r).merged.clubs.length,3);
});
test('insertion collision is rejected; distinct concurrent inserts are kept',()=>{
 const x=base(),l=clone(x),r=clone(x);
 l.teams.push({id:'usb-team',name:'USB'});
 r.teams.push({id:'server-team',name:'Server'});
 assert.equal(merge(x,l,r).merged.teams.length,2);
 r.teams.push({id:'usb-team',name:'Other'});
 assert.equal(merge(x,l,r).ok,false);
});
test('deletion is safe, including already-deleted retry',()=>{
 const x=base(),l=clone(x),r=clone(x);
 l.clubs=l.clubs.filter(y=>y.id!=='b');
 assert.deepEqual(merge(x,l,r).merged.clubs.map(y=>y.id),['a']);
 r.clubs=r.clubs.filter(y=>y.id!=='b');
 assert.equal(merge(x,l,r).applied,0);
});
test('deleting a concurrently changed record is blocked',()=>{
 const x=base(),l=clone(x),r=clone(x);
 l.clubs=l.clubs.filter(y=>y.id!=='a');
 r.clubs[0].name='Modified on server';
 assert.equal(merge(x,l,r).ok,false);
});
test('conflicts in defaults are detected and unrelated keys merge',()=>{
 const x=base(),l=clone(x),r=clone(x);
 l.ageClassFightTimeDefaults.U15=200;
 r.ageClassFightTimeDefaults.U18=240;
 assert.equal(merge(x,l,r).merged.ageClassFightTimeDefaults.U18,240);
 r.ageClassFightTimeDefaults.U15=300;
 assert.equal(merge(x,l,r).ok,false);
});
test('duplicate IDs and malformed masterdata are rejected',()=>{
 const x=base();x.clubs.push({id:'a',name:'duplicate'});
 assert.throws(()=>merge(x,x,x),/doppelte/);
});
test('stable digest ignores object property ordering',()=>{
 assert.equal(digest({a:1,b:{x:2,y:3}}),digest({b:{y:3,x:2},a:1}));
});
