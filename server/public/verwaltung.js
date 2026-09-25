'use strict';
let data={clubs:[],teams:[],fighters:[],competitionDays:[],weightClasses:[],tournamentModes:[],ruleSets:[]}, currentTab='clubs', selectedId=null, modes={};
const $=s=>document.querySelector(s), $$=s=>[...document.querySelectorAll(s)];
const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const toast=t=>{const x=$('#toast');x.textContent=t;x.classList.remove('hidden');setTimeout(()=>x.classList.add('hidden'),2200)};
const clubName=id=>data.clubs.find(x=>x.id===id)?.name||'–';
const teamName=id=>data.teams.find(x=>x.id===id)?.name||'–';
const tournamentModeName=id=>data.tournamentModes.find(x=>x.id===id)?.title||'–';
const fields={
 clubs:[['name','Vereinsname','text'],['shortName','Kurzname','text'],['country','Land','text'],['status','Status','select',['active','inactive']],['website','Webseite','text'],['notes','Bemerkungen','textarea'],['logo','Logo','logo']],
 teams:[['name','Mannschaftsname','text'],['shortName','Kurzname','text'],['clubId','Verein','club'],['category','Kategorie / Liga','text'],['season','Saison','text'],['status','Status','select',['active','inactive']],['notes','Bemerkungen','textarea'],['fighterIds','Kader','roster']],
 fighters:[['firstName','Vorname','text'],['lastName','Nachname','text'],['clubId','Stammverein','club'],['passNumber','Pass-Nr.','text'],['licenseNumber','Lizenz-Nr.','text'],['birthDate','Geburtsdatum','date'],['gender','Geschlecht','select',['','m','w','divers']],['nationality','Nationalität','text'],['weight','Gewicht','number'],['status','Status','select',['active','inactive']],['notes','Bemerkungen','textarea']],
 competitionDays:[['name','Kampftag','text'],['date','Datum','date'],['hostClubId','Ausrichter','club'],['location','Ort','text'],['teamIds','Teilnehmende Mannschaften','teammulti'],['tournamentModeId','Wettkampfmodus','tournamentmode'],['matCount','Anzahl Tatami','number'],['status','Status','select',['planned','active','closed']],['notes','Bemerkungen','textarea']],
 weightClasses:[['name','Bezeichnung','text'],['category','Kategorie','text'],['minWeight','Min. kg','number'],['maxWeight','Max. kg','number'],['order','Reihenfolge','number'],['status','Status','select',['active','inactive']]],
 tournamentModes:[
  ['title','Titel','text'],
  ['subTitle','Untertitel','text'],
  ['weights','Gewichtsklassen (mit ; trennen)','text'],
  ['listTemplate','Vorlage','select',['list_output_nwjv_5_hinundrueck.html','list_output_nwjv_7_hinundrueck.html','list_output_mm.html','list_output_bundesliga.html','list_output_bay.html','list_output_jugendliga.html']],
  ['nRounds','Runden','number'],
  ['fightTimeInSeconds','Kampfzeit in Minuten','minutes'],
  ['weightsAreDoubled','Gewichtsklassen doppelt','checkbox'],
  ['rules','Regelwerk','ruleset'],
  ['fightTimeOverrides','Abweichende Kampfzeiten','text'],
  ['options','Optionen','text']
 ],
 ruleSets:[
  ['name','Bezeichnung','text'],
  ['status','Status','select',['active','inactive']],
  ['hasYuko','Yuko vorhanden','checkbox'],
  ['awaseteIppon','Waza-ari-awasete-Ippon','checkbox'],
  ['openEndGoldenScore','Golden Score ohne Zeitlimit','checkbox'],
  ['shidoAddsPoint','Shido erzeugt Wertung','checkbox'],
  ['shidoScoreCounts','Shido zählt im Vergleich','checkbox'],
  ['maxShidoCount','Maximale Shido vor Hansoku-make','number'],
  ['maxWazaariCount','Waza-ari für Ippon','number'],
  ['osaekomiYukoSeconds','Osaekomi Yuko in Sekunden (0 = aus)','number'],
  ['osaekomiWazaariSeconds','Osaekomi Waza-ari in Sekunden','number'],
  ['osaekomiIpponSeconds','Osaekomi Ippon in Sekunden','number'],
  ['ipponTeamPoints','Unterbewertung Ippon','number'],
  ['wazaariTeamPoints','Unterbewertung Waza-ari','number'],
  ['yukoTeamPoints','Unterbewertung Yuko','number'],
  ['shidoTeamPoints','Unterbewertung Shido','number'],
  ['ipponLabel','Begriff Ippon','text'],
  ['wazaariLabel','Begriff Waza-ari','text'],
  ['yukoLabel','Begriff Yuko','text'],
  ['shidoLabel','Begriff Shido','text'],
  ['hansokumakeLabel','Begriff Hansoku-make','text'],
  ['notes','Bemerkungen','textarea']
 ]
};
const columns={
 clubs:[['logo','Logo'],['name','Vereinsname'],['shortName','Kurzname'],['country','Land'],['status','Status']],
 teams:[['name','Mannschaft'],['clubId','Verein'],['category','Kategorie'],['season','Saison'],['roster','Kader']],
 fighters:[['lastName','Nachname'],['firstName','Vorname'],['clubId','Stammverein'],['passNumber','Pass-Nr.'],['nationality','Nationalität'],['status','Status']],
 competitionDays:[['date','Datum'],['name','Kampftag'],['hostClubId','Ausrichter'],['location','Ort'],['teams','Mannschaften'],['matCount','Tatami'],['status','Status']],
 weightClasses:[['order','#'],['name','Gewichtsklasse'],['category','Kategorie'],['minWeight','Min.'],['maxWeight','Max.'],['status','Status']],
 tournamentModes:[['title','Titel'],['subTitle','Untertitel'],['weights','Gewichtsklassen'],['nRounds','Runden'],['fightTimeInSeconds','Kampfzeit'],['listTemplate','Vorlage']],
 ruleSets:[['name','Regelwerk'],['status','Status'],['maxShidoCount','Shido'],['osaekomiYukoSeconds','Yuko-Zeit'],['osaekomiWazaariSeconds','Waza-ari-Zeit'],['osaekomiIpponSeconds','Ippon-Zeit']]
};
async function api(url,opts){const r=await fetch(url,opts);const j=await r.json();if(!r.ok)throw new Error(j.error||r.statusText);return j}
async function load(){const j=await api('/api/masterdata');data=j.masterdata||{};for(const k of ['clubs','teams','fighters','competitionDays','weightClasses','tournamentModes','ruleSets'])if(!Array.isArray(data[k]))data[k]=[];modes=j.modes||{};$('#sideRevision').textContent='Revision '+(data.revision||0);$('#sideUpdated').textContent='Stand '+new Date(data.updatedAt).toLocaleString('de-DE');render();$('#jsonPreview').value=JSON.stringify({masterdata:data},null,2)}
function setTab(tab){currentTab=tab;selectedId=null;$$('.tabbtn').forEach(b=>b.classList.toggle('active',b.dataset.tab===tab));const imp=tab==='importExport';$('#crudView').classList.toggle('hidden',imp);$('#importView').classList.toggle('hidden',!imp);if(!imp)render()}
function displayValue(row,key){if(key==='clubId')return clubName(row.clubId);if(key==='hostClubId')return clubName(row.hostClubId);if(key==='tournamentModeId')return tournamentModeName(row.tournamentModeId);if(key==='teams')return (row.teamIds||[]).map(teamName).join(', ')||'–';if(key==='fightTimeInSeconds')return `${Number(row[key]||0)/60} min`;if(key==='weightsAreDoubled')return row[key]?'Ja':'Nein';if(key==='roster')return `${(row.fighterIds||[]).length} Kämpfer`;if(key==='logo')return row.logo?`<img class="logo-thumb" src="${esc(row.logo)}" alt="">`:'–';if(key==='status')return `<span class="status-dot" style="background:${row.status==='inactive'?'#777':'#36d66b'}"></span>${esc(row.status||'active')}`;return esc(row[key]??'')}
function render(){if(currentTab==='importExport')return;const q=($('#search')?.value||'').toLowerCase().trim();const rows=(data[currentTab]||[]).filter(r=>JSON.stringify(r).toLowerCase().includes(q));$('#thead').innerHTML='<tr>'+columns[currentTab].map(c=>`<th>${c[1]}</th>`).join('')+'<th>Aktionen</th></tr>';$('#tbody').innerHTML=rows.map(r=>`<tr data-id="${esc(r.id)}" class="${r.id===selectedId?'selected':''}">${columns[currentTab].map(c=>`<td>${displayValue(r,c[0])}</td>`).join('')}<td><div class="actions"><button class="iconbtn edit" data-id="${esc(r.id)}">✎</button><button class="iconbtn del" data-id="${esc(r.id)}">⌫</button></div></td></tr>`).join('')||`<tr><td colspan="${columns[currentTab].length+1}"><div class="empty">Keine Einträge vorhanden.</div></td></tr>`;$$('#tbody tr[data-id]').forEach(tr=>tr.onclick=e=>{if(e.target.closest('button'))return;openEdit(tr.dataset.id)});$$('.edit').forEach(b=>b.onclick=()=>openEdit(b.dataset.id));$$('.del').forEach(b=>b.onclick=()=>remove(b.dataset.id));if(selectedId)showForm((data[currentTab]||[]).find(x=>x.id===selectedId));else $('#detailBody').innerHTML='<div class="empty">Eintrag auswählen oder neu anlegen.</div>'}
function optionsRules(v){return '<option value="">– auswählen –</option>'+data.ruleSets.filter(r=>r.status!=='inactive').map(r=>`<option value="${esc(r.id)}" ${r.id===v?'selected':''}>${esc(r.name||r.id)}</option>`).join('')}
function optionsClub(v){return `<option value="">– auswählen –</option>`+data.clubs.map(c=>`<option value="${esc(c.id)}" ${c.id===v?'selected':''}>${esc(c.name)}</option>`).join('')}
function optionsTournamentMode(v){return `<option value="">– optional –</option>`+data.tournamentModes.map(m=>`<option value="${esc(m.id)}" ${m.id===v?'selected':''}>${esc(m.title||m.id)}</option>`).join('')}

function readMultiValues(el){try{const v=JSON.parse(el&&el.value||'[]');return Array.isArray(v)?v.map(String).filter(Boolean):[]}catch{return []}}
function multiConfig(kind){
 if(kind==='teams')return {title:'Teilnehmende Mannschaften auswählen',items:[...data.teams].filter(x=>x.status!=='inactive').sort((a,b)=>(a.name||'').localeCompare(b.name||'','de')),name:x=>x.name||x.id,sub:x=>[clubName(x.clubId),x.category,x.season].filter(v=>v&&v!=='–').join(' · ')};
 return {title:'Kader auswählen',items:[...data.fighters].filter(x=>x.status!=='inactive').sort((a,b)=>((a.lastName||'')+' '+(a.firstName||'')).localeCompare((b.lastName||'')+' '+(b.firstName||''),'de')),name:x=>((x.firstName||'')+' '+(x.lastName||'')).trim()||x.id,sub:x=>[clubName(x.clubId),x.passNumber?('Pass '+x.passNumber):'',x.nationality||''].filter(v=>v&&v!=='–').join(' · ')};
}
function multiSummary(kind,ids){const cfg=multiConfig(kind),set=new Set(ids||[]),sel=cfg.items.filter(x=>set.has(String(x.id)));if(!sel.length)return 'Keine Auswahl';const names=sel.slice(0,3).map(cfg.name);return sel.length+' ausgewählt: '+names.join(', ')+(sel.length>3?' …':'')}
let multiTarget=null,multiKind='',multiSelected=new Set();
function updateMultiCount(){$('#multiCount').textContent=multiSelected.size+' ausgewählt'}
function renderMultiModal(){
 const cfg=multiConfig(multiKind),q=($('#multiSearch').value||'').toLocaleLowerCase('de-DE').trim();
 const visible=cfg.items.filter(x=>(cfg.name(x)+' '+cfg.sub(x)).toLocaleLowerCase('de-DE').includes(q));
 $('#multiList').innerHTML=visible.map(x=>'<label class="multi-option"><input type="checkbox" value="'+esc(x.id)+'" '+(multiSelected.has(String(x.id))?'checked':'')+'><span><b>'+esc(cfg.name(x))+'</b><small>'+esc(cfg.sub(x))+'</small></span></label>').join('')||'<div class="empty">Keine passenden Einträge.</div>';
 document.querySelectorAll('#multiList input[type="checkbox"]').forEach(cb=>cb.onchange=()=>{cb.checked?multiSelected.add(cb.value):multiSelected.delete(cb.value);updateMultiCount()});
 updateMultiCount();
}
function openMultiModal(kind,target){multiKind=kind;multiTarget=target;multiSelected=new Set(readMultiValues(target));const cfg=multiConfig(kind);$('#multiTitle').textContent=cfg.title;$('#multiSearch').value='';$('#multiModal').classList.remove('hidden');renderMultiModal();setTimeout(()=>$('#multiSearch').focus(),0)}
function closeMultiModal(){$('#multiModal').classList.add('hidden');multiTarget=null;multiKind='';multiSelected=new Set()}
function applyMultiModal(){if(!multiTarget)return closeMultiModal();multiTarget.value=JSON.stringify([...multiSelected]);const summary=multiTarget.parentElement.querySelector('[data-multi-summary]');if(summary)summary.textContent=multiSummary(multiKind,[...multiSelected]);closeMultiModal()}
$('#multiSearch').oninput=renderMultiModal;
$('#multiClose').onclick=closeMultiModal;$('#multiCancel').onclick=closeMultiModal;$('#multiApply').onclick=applyMultiModal;
$('#multiClear').onclick=()=>{multiSelected.clear();renderMultiModal()};
$('#multiSelectVisible').onclick=()=>{const cfg=multiConfig(multiKind),q=($('#multiSearch').value||'').toLocaleLowerCase('de-DE').trim();cfg.items.filter(x=>(cfg.name(x)+' '+cfg.sub(x)).toLocaleLowerCase('de-DE').includes(q)).forEach(x=>multiSelected.add(String(x.id)));renderMultiModal()};
$('#multiModal').onclick=e=>{if(e.target===$('#multiModal'))closeMultiModal()};
window.addEventListener('keydown',e=>{if(e.key==='Escape'&&!$('#multiModal').classList.contains('hidden'))closeMultiModal()});

function showForm(row={}){selectedId=row.id||null;$('#detailTitle').textContent=row.id?'Eintrag bearbeiten':'Neuer Eintrag';let html='';if(currentTab==='clubs'){html+=`<div class="detail-logo"><img id="logoPreview" src="${esc(row.logo||'/assets/branding/ssv_meschede_logo.png')}" alt=""><label class="btn ghost">Logo wählen<input id="logoFile" type="file" accept="image/*" hidden></label></div>`}
 for(const [key,label,type,opts] of fields[currentTab]){if(type==='logo')continue;if(type==='textarea')html+=`<div class="form-row"><label>${label}</label><textarea data-field="${key}">${esc(row[key]||'')}</textarea></div>`;else if(type==='select')html+=`<div class="form-row"><label>${label}</label><select data-field="${key}">${opts.map(o=>`<option value="${esc(o)}" ${String(row[key]||'')===o?'selected':''}>${esc(o||'–')}</option>`).join('')}</select></div>`;else if(type==='ruleset')html+=`<div class="form-row"><label>${label}</label><select data-field="${key}">${optionsRules(row[key])}</select></div>`;else if(type==='minutes')html+=`<div class="form-row"><label>${label}</label><input data-field="${key}" data-unit="minutes" type="number" step="0.5" min="0" value="${esc((Number(row[key]||0)/60)||'')}"></div>`;else if(type==='club')html+=`<div class="form-row"><label>${label}</label><select data-field="${key}">${optionsClub(row[key])}</select></div>`;else if(type==='tournamentmode')html+=`<div class="form-row"><label>${label}</label><select data-field="${key}">${optionsTournamentMode(row[key])}</select></div>`;else if(type==='teammulti'){const ids=Array.isArray(row.teamIds)?row.teamIds:[];html+='<div class="form-row"><label>'+esc(label)+'</label><input type="hidden" data-multiselect="teams" data-field="teamIds" value="'+esc(JSON.stringify(ids))+'"><button type="button" class="btn ghost multi-open" data-multi="teams">Mannschaften auswählen</button><div class="multi-summary" data-multi-summary>'+esc(multiSummary('teams',ids))+'</div></div>'}else if(type==='checkbox')html+=`<div class="form-row"><label>${label}</label><label><input data-field="${key}" type="checkbox" ${row[key]?'checked':''}> Ja</label></div>`;else if(type==='roster'){const ids=Array.isArray(row.fighterIds)?row.fighterIds:[];html+='<div class="form-row"><label>'+esc(label)+'</label><input type="hidden" data-multiselect="fighters" data-field="fighterIds" value="'+esc(JSON.stringify(ids))+'"><button type="button" class="btn ghost multi-open" data-multi="fighters">Kader auswählen</button><div class="multi-summary" data-multi-summary>'+esc(multiSummary('fighters',ids))+'</div></div>'}else html+=`<div class="form-row"><label>${label}</label><input data-field="${key}" type="${type}" value="${esc(row[key]??'')}"></div>`}
 html+=`<div class="form-actions"><button id="saveBtn" class="btn red">Änderungen speichern</button><button id="cancelBtn" class="btn ghost">Abbrechen</button></div>`;if(row.id)html+=`<div class="delete-zone"><button id="deleteDetailBtn" class="btn danger">Eintrag löschen</button></div>`;$('#detailBody').innerHTML=html;$('#saveBtn').onclick=saveForm;$('#cancelBtn').onclick=()=>{selectedId=null;render()};const deleteDetailBtn=$('#deleteDetailBtn');if(deleteDetailBtn)deleteDetailBtn.onclick=()=>remove(row.id);const lf=$('#logoFile');if(lf)lf.onchange=async()=>{const f=lf.files[0];if(!f)return;if(f.size>2_000_000){toast('Logo maximal 2 MB');return}const reader=new FileReader();reader.onload=()=>{$('#logoPreview').src=reader.result;$('#logoPreview').dataset.newLogo=reader.result};reader.readAsDataURL(f)};document.querySelectorAll('#detailBody .multi-open').forEach(b=>b.onclick=()=>{const target=b.parentElement.querySelector('[data-multiselect]');openMultiModal(b.dataset.multi,target)})}
function openEdit(id){selectedId=id;render();showForm((data[currentTab]||[]).find(x=>x.id===id)||{})}
async function saveForm(){const old=(data[currentTab]||[]).find(x=>x.id===selectedId)||{};const rec={...old};document.querySelectorAll('#detailBody [data-field]:not([data-multiselect])').forEach(el=>{rec[el.dataset.field]=el.type==='checkbox'?el.checked:(el.dataset.unit==='minutes'?(el.value===''?'':Math.round(Number(el.value)*60)):(el.type==='number'?(el.value===''?'':Number(el.value)):el.value))});document.querySelectorAll('#detailBody [data-multiselect]').forEach(el=>{rec[el.dataset.field]=readMultiValues(el)});const lp=$('#logoPreview');if(currentTab==='clubs'&&lp?.dataset.newLogo)rec.logo=lp.dataset.newLogo;try{await api('/api/masterdata/'+currentTab,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(rec)});toast('Gespeichert');selectedId=null;await load()}catch(e){toast('Fehler: '+e.message)}}
function deletePrompt(id){
 const row=(data[currentTab]||[]).find(x=>x.id===id)||{};
 const labels={clubs:'Verein',teams:'Mannschaft',fighters:'Wettkämpfer',competitionDays:'Kampftag',weightClasses:'Gewichtsklasse',tournamentModes:'Wettkampfmodus',ruleSets:'Regelwerk'};
 const type=labels[currentTab]||'Eintrag';
 const name=row.name||row.title||row.firstName&&row.lastName&&(row.firstName+' '+row.lastName)||id;
 let extra='';
 if(currentTab==='clubs'){
  const teams=data.teams.filter(x=>x.clubId===id).length,fighters=data.fighters.filter(x=>x.clubId===id).length;
  if(teams||fighters)extra='\n\nAchtung: Zugeordnete Mannschaften ('+teams+') und Wettkämpfer ('+fighters+') werden ebenfalls gelöscht.';
 }
 if(currentTab==='teams')extra='\n\nDie Mannschaft wird auch aus allen Kampftagen entfernt.';
 if(currentTab==='fighters')extra='\n\nDer Wettkämpfer wird auch aus allen Mannschaftskadern entfernt.';
 if(currentTab==='tournamentModes')extra='\n\nDer Wettkampfmodus wird aus verknüpften Kampftagen entfernt.';
 if(currentTab==='ruleSets')extra='\n\nDas Regelwerk wird aus verknüpften Wettkampfmodi entfernt.';
 return type+' „'+name+'“ wirklich dauerhaft löschen?'+extra;
}
async function remove(id){if(!id||!confirm(deletePrompt(id)))return;try{await api('/api/masterdata/'+currentTab+'/'+encodeURIComponent(id),{method:'DELETE'});toast('Gelöscht');selectedId=null;await load()}catch(e){toast('Fehler: '+e.message)}}
$('#tabs').onclick=e=>{const b=e.target.closest('[data-tab]');if(b)setTab(b.dataset.tab)};
$('#newBtn').onclick=()=>{selectedId=null;showForm(currentTab==='tournamentModes'?{
 title:'Neuer Wettkampfmodus',
 subTitle:'',
 weights:'-66kg;-73kg;-81kg;-90kg;+90kg',
 listTemplate:'list_output_nwjv_5_hinundrueck.html',
 nRounds:2,
 fightTimeInSeconds:240,
 weightsAreDoubled:false,
 rules:'IJF-2025',
 fightTimeOverrides:'',
 options:''
}:currentTab==='ruleSets'?{
 name:'Neues Regelwerk',status:'active',hasYuko:true,awaseteIppon:true,openEndGoldenScore:true,shidoAddsPoint:false,shidoScoreCounts:false,maxShidoCount:2,maxWazaariCount:2,osaekomiYukoSeconds:5,osaekomiWazaariSeconds:10,osaekomiIpponSeconds:20,ipponTeamPoints:10,wazaariTeamPoints:7,yukoTeamPoints:5,shidoTeamPoints:1,ipponLabel:'Ippon',wazaariLabel:'Waza-ari',yukoLabel:'Yuko',shidoLabel:'Shido',hansokumakeLabel:'Hansoku-make',notes:''
}:currentTab==='competitionDays'?{
 name:'Neuer Kampftag',date:'',hostClubId:'',location:'',teamIds:[],tournamentModeId:'',matCount:1,status:'planned',notes:''
}:{status:'active'})};$('#closeDetail').onclick=()=>{selectedId=null;render()};$('#search').oninput=render;$('#syncBtn').onclick=async()=>{await load();toast('Daten neu geladen')};
$('#exportBtn').onclick=()=>{const blob=new Blob([JSON.stringify({masterdata:data},null,2)],{type:'application/json'}),a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download='Ipponboard-Meschede_Stammdaten.json';a.click();URL.revokeObjectURL(a.href)};$('#copyBtn').onclick=async()=>{await navigator.clipboard.writeText($('#jsonPreview').value);toast('In Zwischenablage kopiert')};$('#importFile').onchange=async e=>{const f=e.target.files[0];if(f)$('#jsonPreview').value=await f.text()};$('#applyImportBtn').onclick=async()=>{try{const x=JSON.parse($('#jsonPreview').value);await api('/api/masterdata/import',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(x)});toast('Importiert');await load();setTab('clubs')}catch(e){toast('Importfehler: '+e.message)}};
load().catch(e=>{console.error(e);$('#sideOnline').textContent='Serverfehler';$('#sideOnline').className='warn';toast('Daten konnten nicht geladen werden')});

async function exportXlsx(type){
 try{
  const r=await fetch('/api/xlsx/'+encodeURIComponent(type));
  if(!r.ok){let msg=r.statusText;try{msg=(await r.json()).error||msg}catch{}throw new Error(msg)}
  const blob=await r.blob();
  const disposition=r.headers.get('Content-Disposition')||'';
  const match=disposition.match(/filename="?([^"]+)"?/i);
  const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=match?match[1]:'Ipponboard_'+type+'.xlsx';a.click();URL.revokeObjectURL(a.href);
 }catch(e){toast('XLSX-Export: '+e.message)}
}
async function importXlsx(type,file){
 if(!file)return;
 if(!confirm('XLSX-Datei importieren? Datensätze mit gleicher ID werden aktualisiert. Mit „Löschen = JA“ markierte Datensätze werden gelöscht.'))return;
 try{
  const r=await fetch('/api/xlsx/'+encodeURIComponent(type),{method:'POST',headers:{'Content-Type':'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'},body:file});
  const j=await r.json();if(!r.ok)throw new Error(j.error||r.statusText);
  const summary=(j.created||0)+' neu, '+(j.updated||0)+' geändert, '+(j.deleted||0)+' gelöscht';
  toast('XLSX importiert: '+summary);
  if(Array.isArray(j.warnings)&&j.warnings.length)alert('Import abgeschlossen mit Hinweisen:\n\n'+j.warnings.slice(0,20).join('\n')+(j.warnings.length>20?'\n… weitere Hinweise':''));
  await load();
 }catch(e){toast('XLSX-Import: '+e.message)}
}
$('.xlsx-export').forEach(b=>b.onclick=()=>exportXlsx(b.dataset.type));
$('.xlsx-import').forEach(i=>i.onchange=async()=>{const f=i.files&&i.files[0];await importXlsx(i.dataset.type,f);i.value=''});

async function exportRegistrationTemplate(type){
 try{
  const r=await fetch('/api/registration-template/'+encodeURIComponent(type));
  if(!r.ok){let msg=r.statusText;try{msg=(await r.json()).error||msg}catch{}throw new Error(msg)}
  const blob=await r.blob(),disposition=r.headers.get('Content-Disposition')||'';
  const match=disposition.match(/filename="?([^"]+)"?/i);
  const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=match?match[1]:'Ipponboard_Meldeliste.xlsx';a.click();URL.revokeObjectURL(a.href);
 }catch(e){toast('Blanko-Liste: '+e.message)}
}
async function importRegistrationTemplate(type,file){
 if(!file)return;
 if(!confirm('Ausgefüllte Meldeliste einlesen? Vorhandene Personen werden nur bei eindeutiger Übereinstimmung aktualisiert.'))return;
 try{
  const r=await fetch('/api/registration-template/'+encodeURIComponent(type),{method:'POST',headers:{'Content-Type':'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'},body:file});
  const j=await r.json();if(!r.ok)throw new Error(j.error||r.statusText);
  const parts=[];
  if(j.clubName)parts.push('Verein: '+j.clubName);
  if(j.teamName)parts.push('Mannschaft: '+j.teamName);
  parts.push((j.fightersCreated||0)+' Wettkämpfer neu');
  parts.push((j.fightersUpdated||0)+' aktualisiert');
  if(j.teamCreated)parts.push('Mannschaft neu angelegt');
  if(j.teamUpdated)parts.push('Kader aktualisiert');
  toast('Meldeliste importiert');
  const warnings=Array.isArray(j.warnings)?j.warnings:[];
  alert(parts.join('\n')+(warnings.length?'\n\nHinweise:\n'+warnings.slice(0,25).join('\n')+(warnings.length>25?'\n… weitere Hinweise':''):''));
  await load();
 }catch(e){toast('Meldelisten-Import: '+e.message)}
}
$('.registration-export').forEach(b=>b.onclick=()=>exportRegistrationTemplate(b.dataset.type));
$('.registration-import').forEach(i=>i.onchange=async()=>{const f=i.files&&i.files[0];await importRegistrationTemplate(i.dataset.type,f);i.value=''});


const nwjvBtn=$('#nwjvImportBtn');
if(nwjvBtn)nwjvBtn.onclick=async()=>{
 if(!confirm('NWJV Bezirksliga Männer Arnsberg 2026 als Testdaten ergänzen? Vorhandene andere Stammdaten bleiben erhalten.'))return;
 nwjvBtn.disabled=true;
 try{
  const seed=await api('/data/nwjv-bezirksliga-arnsberg-2026.json');
  await api('/api/masterdata/merge',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(seed)});
  toast('NWJV-Testdaten ergänzt');
  await load();
  setTab('teams');
 }catch(e){toast('Importfehler: '+e.message)}
 finally{nwjvBtn.disabled=false}
};
