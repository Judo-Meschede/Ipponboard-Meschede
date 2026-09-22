let state,modes,ws;const $=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)];
const team=id=>state.teams.find(t=>t.id===id),match=()=>state.matches[state.matchIndex],fight=()=>state.fights[state.fightIndex];
async function api(a){await fetch('/api/action',{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify(a)})}
function fmt(ms){const s=Math.max(0,Math.ceil(ms/1000));return `${Math.floor(s/60)}:${String(s%60).padStart(2,'0')}`}
function div(cls,txt=''){const n=document.createElement('div');n.className=cls;if(txt!==undefined)n.textContent=txt;return n}
function fit(el){if(!el||!el.textContent)return;let lo=1,hi=300;const cs=getComputedStyle(el),probe=document.createElement('span');probe.textContent=el.textContent;Object.assign(probe.style,{position:'absolute',visibility:'hidden',whiteSpace:'nowrap',fontFamily:cs.fontFamily,fontWeight:cs.fontWeight,fontStyle:cs.fontStyle,lineHeight:'1'});document.body.append(probe);const w=Math.max(1,el.clientWidth*.96),h=Math.max(1,el.clientHeight*.92);for(let i=0;i<10;i++){const m=(lo+hi)/2;probe.style.fontSize=m+'px';const r=probe.getBoundingClientRect();if(r.width<=w&&r.height<=h)lo=m;else hi=m}el.style.fontSize=Math.max(1,lo)+'px';probe.remove()}
function addScaled(parent,cls,txt){const n=div('scaled '+cls,txt);parent.append(n);return n}
function sideClass(side){return side==='white'?'side-white':'side-blue'}
function scoreSideObj(side,f){return side==='white'?f.white:f.blue}
function sideName(side,f){return side==='white'?f.whiteName:f.blueName}
function penaltyCell(parent,side,on,hanso=false,secondary=false){const n=div('pen '+sideClass(side)+(on?' on':' off'));const im=new Image();im.src=`/assets/original/${hanso?(on?'on_hansokumake':'off_hansokumake'):(on?'on':'off')}.png`;n.append(im);parent.append(n)}
function buildPointHalf(side,obj,secondary){const h=div('sv-point-half '+sideClass(side));if(secondary){
  if(obj.i){addScaled(h,'score-digit '+sideClass(side),'IPPON');h.style.gridTemplateColumns='1fr';}
  else{addScaled(h,'score-digit '+sideClass(side),String(obj.w||0));addScaled(h,'score-digit '+sideClass(side),String(obj.y||0));}
}else{addScaled(h,'score-digit '+sideClass(side),obj.i?'IPPON':'-');addScaled(h,'score-digit '+sideClass(side),String(obj.w||0));addScaled(h,'score-digit '+sideClass(side),String(obj.y||0));}
 return h}
function buildPointNames(side,obj,secondary){const h=div('sv-point-half '+sideClass(side));if(secondary){
  if(obj.i){addScaled(h,'pointdesc '+sideClass(side),'');h.style.gridTemplateColumns='1fr';}
  else{addScaled(h,'pointdesc '+sideClass(side),'Waza-ari');addScaled(h,'pointdesc '+sideClass(side),'Yuko');}
}else{addScaled(h,'pointdesc '+sideClass(side),'I');addScaled(h,'pointdesc '+sideClass(side),'W');addScaled(h,'pointdesc '+sideClass(side),'Y');}
 return h}
function buildBoard(){const root=$('#board');if(!root||!state)return;root.innerHTML='';const secondary=root.classList.contains('secondary'),f=fight(),m=match();
 const left=secondary?'blue':'white',right=secondary?'white':'blue',L=scoreSideObj(left,f),R=scoreSideObj(right,f);
 const lt=left==='white'?team(m.home):team(m.guest),rt=right==='white'?team(m.home):team(m.guest);
 const header=div('sv-row sv-header');addScaled(header,'info left','Ipponboard');addScaled(header,'info right',f.weight||'');root.append(header);
 const holdActive=!!f.hold.active;
 const info=div('sv-row sv-info'+(holdActive?' hold-active':''));
 const ln=div('sv-name '+sideClass(left));addScaled(ln,'fighter-name '+sideClass(left),sideName(left,f)||'');addScaled(ln,'fighter-name '+sideClass(left),lt?.name||'');info.append(ln);
 if(holdActive){const hold=div('sv-hold');const hcL=addScaled(hold,'hold-clock','');const himg=div('hold-img');const hi=new Image();hi.src='/assets/original/sand_clock.png';himg.append(hi);hold.append(himg);const hcR=addScaled(hold,'hold-clock','');const sec=String(Math.floor(f.hold.timeMs/1000)).padStart(2,'0');if(f.hold.side===left){hcL.textContent=sec;hcL.classList.add(sideClass(left));himg.style.background=left==='white'?'#fff':'#0000ff'}else{hcR.textContent=sec;hcR.classList.add(sideClass(right));himg.style.background=right==='white'?'#fff':'#0000ff'}info.append(hold)}
 const rn=div('sv-name '+sideClass(right));addScaled(rn,'fighter-name '+sideClass(right),sideName(right,f)||'');addScaled(rn,'fighter-name '+sideClass(right),rt?.name||'');info.append(rn);root.append(info);
 const pn=div('sv-row sv-pointnames');pn.append(buildPointNames(left,L,secondary),buildPointNames(right,R,secondary));root.append(pn);
 const scores=div('sv-row sv-scores');scores.append(buildPointHalf(left,L,secondary),buildPointHalf(right,R,secondary));root.append(scores);
 const pens=div('sv-row sv-penalties');for(let i=0;i<3;i++)penaltyCell(pens,left,L.s>i,false,secondary);penaltyCell(pens,left,L.h>0,true,secondary);penaltyCell(pens,left,false,false,secondary);penaltyCell(pens,right,false,false,secondary);penaltyCell(pens,right,R.h>0,true,secondary);for(let i=2;i>=0;i--)penaltyCell(pens,right,R.s>i,false,secondary);root.append(pens);
 const bottom=div('sv-row sv-bottom');const lts=div(sideClass(left));lts.style.display='grid';lts.style.gridTemplateRows='2fr 3fr';addScaled(lts,'team-score '+sideClass(left),lt?.name||'');addScaled(lts,'team-score '+sideClass(left),String(state.teamScore[lt?.id]||0));bottom.append(lts);bottom.append(div(''));addScaled(bottom,'clock',fmt(f.timeMs));const rts=div(sideClass(right));rts.style.display='grid';rts.style.gridTemplateRows='2fr 3fr';addScaled(rts,'team-score '+sideClass(right),rt?.name||'');addScaled(rts,'team-score '+sideClass(right),String(state.teamScore[rt?.id]||0));bottom.append(rts);root.append(bottom);
 requestAnimationFrame(()=>$$('#board .scaled').forEach(fit));bindBoardClicks();}
function sendScore(side,key,delta){api({type:'score',side,key,delta})}
function holdAction(side){const f=fight();if(!f.hold.active)return api({type:'hold',action:'start',side});if(f.hold.side!==side)return api({type:'hold',action:'switch'});return api({type:'hold',action:'stop'})}
function bindBoardClicks(){const root=$('#board');if(!root||root.classList.contains('secondary'))return;const rows=root.children,scoreRow=rows[3],penRow=rows[4],bottom=rows[5],infoRow=rows[1];const sides=['white','blue'];
 [...scoreRow.children].forEach((half,idx)=>[...half.children].forEach((c,k)=>{const side=sides[idx],key=['i','w','y'][k];c.onclick=()=>sendScore(side,key,1);c.oncontextmenu=e=>{e.preventDefault();sendScore(side,key,-1)}}));
 const penCells=[...penRow.children];penCells.forEach((c,i)=>{let side=null,key=null;if(i<=2){side='white';key='s'}else if(i===3){side='white';key='h'}else if(i===6){side='blue';key='h'}else if(i>=7){side='blue';key='s'}if(!side)return;c.onclick=()=>sendScore(side,key,1);c.oncontextmenu=e=>{e.preventDefault();sendScore(side,key,-1)}});
 if(bottom&&bottom.children[2]){bottom.children[2].onclick=()=>api({type:'clock',action:'toggle'});bottom.children[2].oncontextmenu=e=>{e.preventDefault();api({type:'clock',action:'reset'})}}
 if(infoRow){infoRow.onclick=e=>{const r=infoRow.getBoundingClientRect(),x=(e.clientX-r.left)/r.width;if(x>=.34&&x<.5)holdAction('white');else if(x>=.5&&x<=.66)holdAction('blue')};infoRow.oncontextmenu=e=>{e.preventDefault();const r=infoRow.getBoundingClientRect(),x=(e.clientX-r.left)/r.width;if(x>=.34&&x<=.66)api({type:'hold',action:'reset'})}}
}
function bindKeyboard(){if(!document.body.classList.contains('operator'))return;window.addEventListener('keydown',e=>{const revoke=e.ctrlKey||e.metaKey;let handled=true;switch(e.key){
 case ' ': api({type:'clock',action:'toggle'});break;
 case 'ArrowLeft': holdAction('white');break;
 case 'ArrowRight': holdAction('blue');break;
 case 'ArrowDown': api({type:'hold',action:'reset'});break;
 case 'F5': sendScore('white','i',revoke?-1:1);break;
 case 'F6': sendScore('white','w',revoke?-1:1);break;
 case 'F7': sendScore('white','y',revoke?-1:1);break;
 case 'F8': sendScore('white','s',revoke?-1:1);break;
 case 'F9': sendScore('blue','i',revoke?-1:1);break;
 case 'F10': sendScore('blue','w',revoke?-1:1);break;
 case 'F11': sendScore('blue','y',revoke?-1:1);break;
 case 'F12': sendScore('blue','s',revoke?-1:1);break;
 case 'Backspace': if(revoke)api({type:'reset-current'});else handled=false;break;
 default: handled=false;
 }if(handled)e.preventDefault()},{capture:true})}
function renderOperator(){if(!document.body.classList.contains('operator'))return;$('#fightNo').textContent=`${state.fightIndex+1} / ${state.fights.length}`;$('#resetBtn').onclick=()=>api({type:'reset-current'});$$('[data-act]').forEach(b=>b.onclick=()=>api({type:b.dataset.act}));}
function updateFastState(){const root=$('#board');if(!root||!state)return false;const f=fight(),clock=root.querySelector('.clock');if(clock)clock.textContent=fmt(f.timeMs);const info=root.querySelector('.sv-info');if(info){const holdActive=!!f.hold.active;if(holdActive!==info.classList.contains('hold-active'))return false;const hold=root.querySelector('.sv-hold');if(holdActive&&hold){const secondary=root.classList.contains('secondary'),left=secondary?'blue':'white',right=secondary?'white':'blue',clocks=hold.querySelectorAll('.hold-clock'),img=hold.querySelector('.hold-img');clocks[0].textContent='';clocks[1].textContent='';clocks[0].className='scaled hold-clock';clocks[1].className='scaled hold-clock';const sec=String(Math.floor(f.hold.timeMs/1000)).padStart(2,'0');if(f.hold.side===left){clocks[0].textContent=sec;clocks[0].classList.add(sideClass(left));img.style.background=left==='white'?'#fff':'#0000ff'}else{clocks[1].textContent=sec;clocks[1].classList.add(sideClass(right));img.style.background=right==='white'?'#fff':'#0000ff'}}}return true}
function structuralKey(st){const c=JSON.parse(JSON.stringify(st));if(c.fights)for(const f of c.fights){delete f.timeMs;delete f.running;if(f.hold)delete f.hold.timeMs}delete c.updatedAt;return JSON.stringify(c)}
async function start(){const r=await fetch('/api/state').then(r=>r.json());state=r.state;modes=r.modes;buildBoard();renderOperator();bindKeyboard();ws=new WebSocket(`${location.protocol==='https:'?'wss':'ws'}://${location.host}/ws`);let lastStructure=structuralKey(state);ws.onmessage=e=>{const m=JSON.parse(e.data);if(m.type==='state'){const next=m.state,k=structuralKey(next);state=next;if(k===lastStructure){if(!updateFastState()){buildBoard();renderOperator()}}else{lastStructure=k;buildBoard();renderOperator()}}};window.addEventListener('resize',()=>requestAnimationFrame(()=>$$('#board .scaled').forEach(fit)));if(document.body.classList.contains('display')){const b=$('#fullscreen'),isElectron=new URLSearchParams(location.search).get('electron')==='1';if(b&&!isElectron){b.onclick=async()=>{try{await document.documentElement.requestFullscreen({navigationUI:'hide'})}catch{}};document.addEventListener('fullscreenchange',()=>document.body.classList.toggle('is-fullscreen',!!document.fullscreenElement));}else if(b)b.remove();}}
start();
