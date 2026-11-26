// WebPage.cpp - the single-page browser UI, embedded so the program needs no extra files.
#include <string>

std::string webPageHtml() {
    return R"WEBPAGE(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Privilege Tetris</title>
<style>
:root{--bg:#f3f4fb;--card:#fff;--ink:#1d2140;--mute:#6b7194;--line:#e4e6f3;--accent:#5b5fe3;--accent2:#8a63f0;--accent-ink:#fff;
--side:#1b1e3d;--side-ink:#c9cdf0;--side-on:#2c3066;
--ok:#14804a;--okbg:#e0f6ea;--warn:#8f5b00;--warnbg:#fff0d1;--bad:#c0281f;--badbg:#fde7e5;--chip:#eef0fa;--on:#dfe1ff;--shadow:0 2px 14px rgba(40,44,100,.08);}
@media (prefers-color-scheme:dark){:root{--bg:#12142a;--card:#1c1f3b;--ink:#e9ebfb;--mute:#9da3cc;--line:#2c3058;--accent:#8187ff;--accent2:#a487ff;--accent-ink:#10122a;
--side:#0d0f22;--side-ink:#aab0dc;--side-on:#22264d;
--ok:#5fd08a;--okbg:#14301f;--warn:#f0c15a;--warnbg:#33290f;--bad:#ff8a80;--badbg:#3a1a18;--chip:#262a4d;--on:#31387a;--shadow:0 2px 14px rgba(0,0,0,.35);}}
*{box-sizing:border-box}
html{scroll-behavior:smooth}
body{margin:0;background:var(--bg);color:var(--ink);font:14px/1.5 "Segoe UI",system-ui,-apple-system,Roboto,sans-serif}
aside{position:fixed;inset:0 auto 0 0;width:230px;background:var(--side);color:var(--side-ink);display:flex;flex-direction:column;padding:18px 12px;z-index:10}
.brand{display:flex;align-items:center;gap:10px;color:#fff;font-weight:700;font-size:17px;padding:4px 10px 18px}
.logo{width:34px;height:34px;border-radius:10px;background:linear-gradient(135deg,var(--accent),var(--accent2));display:grid;grid-template-columns:repeat(2,1fr);gap:3px;padding:7px}
.logo i{background:#fff;border-radius:2px;opacity:.95}.logo i:nth-child(2){opacity:.55}.logo i:nth-child(3){opacity:.55}
nav a{display:flex;align-items:center;gap:10px;color:var(--side-ink);text-decoration:none;padding:9px 12px;border-radius:9px;margin:2px 0}
nav a:hover{background:var(--side-on);color:#fff}
nav a.on{background:linear-gradient(90deg,var(--accent),var(--accent2));color:#fff;font-weight:600}
nav a b{width:22px;text-align:center;font-weight:700;opacity:.9}
nav small{display:block;margin:16px 12px 4px;font-size:11px;letter-spacing:.08em;text-transform:uppercase;opacity:.6}
.side-foot{margin-top:auto;font-size:12px;opacity:.65;padding:0 10px}
.wrap{margin-left:230px}
header{display:flex;flex-wrap:wrap;gap:10px;align-items:center;padding:12px 24px;background:var(--card);border-bottom:1px solid var(--line);position:sticky;top:0;z-index:5;box-shadow:var(--shadow)}
header h1{font-size:18px;margin:0 10px 0 0}
header .sp{flex:1}
#stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:16px;padding:20px 24px 0;max-width:1500px;margin:0 auto}
.stat{background:var(--card);border-radius:14px;padding:14px 16px;box-shadow:var(--shadow);border:1px solid var(--line);display:flex;gap:12px;align-items:center}
.ico{width:42px;height:42px;border-radius:12px;display:grid;place-items:center;font-weight:700;font-size:17px;color:#fff;flex:none}
.stat .v{font-size:24px;font-weight:700;line-height:1.1}.stat .l{color:var(--mute);font-size:12.5px}
main{display:grid;grid-template-columns:minmax(340px,1fr) minmax(420px,1.25fr);gap:20px;padding:20px 24px;max-width:1500px;margin:0 auto}
@media (max-width:1100px){main{grid-template-columns:1fr}}
@media (max-width:860px){aside{position:static;width:auto;flex-direction:row;flex-wrap:wrap;align-items:center;padding:8px}.wrap{margin-left:0}nav small,.side-foot{display:none}.brand{padding:4px 10px}nav{display:flex;flex-wrap:wrap}}
section.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px 18px;margin-bottom:20px;box-shadow:var(--shadow);scroll-margin-top:80px}
h2{font-size:16px;margin:0 0 12px;display:flex;align-items:center;gap:8px}
h2:before{content:"";width:4px;height:16px;border-radius:3px;background:linear-gradient(var(--accent),var(--accent2))}
h3{font-size:12px;margin:14px 0 6px;color:var(--mute);text-transform:uppercase;letter-spacing:.05em}
button,select,input,textarea{font:inherit;color:inherit}
button{background:var(--chip);border:1px solid var(--line);border-radius:9px;padding:6px 13px;cursor:pointer;transition:.12s}
button:hover{border-color:var(--accent);transform:translateY(-1px)}
button.primary{background:linear-gradient(90deg,var(--accent),var(--accent2));color:#fff;border-color:transparent;font-weight:600}
button.danger{color:var(--bad)}
button:disabled{opacity:.5;cursor:not-allowed;transform:none}
button:focus-visible,select:focus-visible,input:focus-visible,textarea:focus-visible,summary:focus-visible,a:focus-visible,.chip:focus-visible{outline:2px solid var(--accent);outline-offset:2px}
input[type=text],input[type=number],select,textarea{background:var(--card);border:1px solid var(--line);border-radius:8px;padding:5px 9px}
input[type=number]{width:66px}
textarea{width:100%;min-height:300px;font:12.5px/1.4 ui-monospace,Menlo,Consolas,monospace}
table{border-collapse:collapse;width:100%}
th,td{text-align:left;padding:7px 9px;border-bottom:1px solid var(--line);vertical-align:top}
th{font-size:12px;color:var(--mute);font-weight:600;background:var(--chip)}
th:first-child{border-radius:8px 0 0 8px}th:last-child{border-radius:0 8px 8px 0}
.row{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin:6px 0}
.mute{color:var(--mute)}
.chips{display:flex;flex-wrap:wrap;gap:6px;margin:3px 0 8px}
.chip{background:var(--chip);border:1px solid var(--line);border-radius:99px;padding:2px 11px;font-size:12.5px;cursor:pointer;user-select:none}
.chip.on{background:var(--on);border-color:var(--accent);font-weight:600}
.chip.static{cursor:default}
.tag{display:inline-block;border-radius:6px;padding:0 8px;font-size:12px;font-weight:700}
.CRITICAL,.bad{background:var(--badbg);color:var(--bad)}
.HIGH,.warn{background:var(--warnbg);color:var(--warn)}
.MEDIUM{background:var(--chip);color:var(--ink)}.LOW{background:var(--chip);color:var(--mute)}
.good{background:var(--okbg);color:var(--ok)}
.box{border:1px solid var(--line);border-radius:10px;padding:10px 12px;margin:8px 0}
.box.err{border-color:var(--bad);background:var(--badbg);color:var(--bad)}
.box.note{background:var(--chip)}
.box.good{border-color:transparent}.box.warn{border-color:transparent}
.scorewrap{display:flex;gap:18px;flex-wrap:wrap}
.score{flex:1;min-width:170px}
.score .n{font-size:34px;font-weight:700;line-height:1.1}
.bar{height:10px;border-radius:6px;background:var(--chip);overflow:hidden;margin:6px 0}
.bar>i{display:block;height:100%;border-radius:6px}
.add{color:var(--ok);font-weight:600}.rem{color:var(--bad);font-weight:600;text-decoration:line-through}
details{margin:6px 0}summary{cursor:pointer;font-weight:600}
.role,.user,.sod{border:1px solid var(--line);border-radius:12px;padding:10px 12px;margin:10px 0}
.role .head,.user .head,.sod .head{display:flex;gap:8px;align-items:center;justify-content:space-between}
code{font:12.5px ui-monospace,Menlo,Consolas,monospace;background:var(--chip);padding:0 5px;border-radius:5px}
.tabs{display:flex;gap:6px;margin-bottom:12px;flex-wrap:wrap;border-bottom:1px solid var(--line);padding-bottom:10px}
.tabs button[aria-selected=true]{background:linear-gradient(90deg,var(--accent),var(--accent2));color:#fff;border-color:transparent}
.sr{position:absolute;left:-9999px}
.hero{max-width:1500px;margin:20px auto 0;padding:0 24px}
.hero>div{border-radius:18px;padding:26px 30px;color:#fff;background:radial-gradient(900px 300px at 85% -20%,rgba(255,255,255,.22),transparent 60%),linear-gradient(120deg,#4a4fe0,#7a55ea 55%,#b45bd8);box-shadow:0 10px 30px rgba(91,95,227,.35);display:flex;gap:24px;flex-wrap:wrap;align-items:center;justify-content:space-between}
.hero h2{color:#fff;font-size:24px;margin:0 0 6px;display:block}.hero h2:before{display:none}
.hero p{margin:0;max-width:560px;opacity:.92}
.steps{display:flex;gap:6px;align-items:center;flex-wrap:wrap}
.step{display:flex;align-items:center;gap:8px;background:rgba(255,255,255,.14);border-radius:99px;padding:6px 14px 6px 7px;font-weight:600;font-size:13px}
.step i{font-style:normal;width:24px;height:24px;border-radius:50%;display:grid;place-items:center;background:rgba(255,255,255,.28);font-size:12px}
.step.done i{background:#3ddc84;color:#08301a}.step.fail i{background:#ff6b61;color:#3a0804}
.step+.step:before{content:"";}
.gauges{display:flex;gap:22px;flex-wrap:wrap;align-items:center;justify-content:space-around}
.gauge{text-align:center}.gauge svg{display:block;margin:0 auto 4px}
.gauge .r{font-weight:700;font-size:13px}.gauge .t{color:var(--mute);font-size:12.5px}
.arrow{font-size:28px;color:var(--mute)}
.stack{display:flex;height:16px;border-radius:9px;overflow:hidden;background:var(--chip);margin:6px 0}
.stack i{display:block;height:100%}
.legend{display:flex;gap:14px;flex-wrap:wrap;font-size:12.5px;color:var(--mute);margin-bottom:8px}
.legend b{display:inline-block;width:10px;height:10px;border-radius:3px;margin-right:5px;vertical-align:-1px}
.ubar{display:grid;grid-template-columns:110px 1fr 46px;gap:10px;align-items:center;margin:6px 0}
.ubar .bar{margin:0}
.matrix{overflow-x:auto}
.matrix table{min-width:100%}
.matrix th.pc{writing-mode:vertical-rl;transform:rotate(180deg);height:120px;text-align:left;vertical-align:bottom;font-weight:600;padding:6px 4px}
.matrix td.c{text-align:center;padding:5px;font-size:15px;width:34px}
.m-ok{color:var(--accent)}.m-bad{color:var(--bad);font-weight:800}.m-warn{color:var(--warn);font-weight:800}.m-miss{color:var(--bad)}
.tree{white-space:pre;font:12.5px/1.45 ui-monospace,Menlo,Consolas,monospace;background:var(--chip);border-radius:10px;padding:10px 12px;overflow:auto}
.stat{transition:.15s}.stat:hover{transform:translateY(-2px)}
section.card{transition:box-shadow .15s}section.card:hover{box-shadow:0 6px 22px rgba(40,44,100,.14)}
@keyframes grow{from{stroke-dasharray:0 999}}
.gauge circle.v{animation:grow .8s ease-out}
@media (prefers-reduced-motion:reduce){html{scroll-behavior:auto}button{transition:none}}
</style>
</head>
<body>
<aside>
  <div class="brand"><span class="logo" aria-hidden="true"><i></i><i></i><i></i><i></i></span>Privilege Tetris</div>
  <nav aria-label="Sections">
    <small>Workspace</small>
    <a href="#sec-build" class="on"><b>▦</b>Configuration</a>
    <a href="#sec-settings"><b>⚙</b>Repair settings</a>
    <small>Analysis</small>
    <a href="#sec-compliance"><b>◔</b>Compliance</a>
    <a href="#sec-matrix"><b>▤</b>Privilege matrix</a>
    <a href="#sec-violations"><b>!</b>Violations</a>
    <a href="#sec-repair"><b>✦</b>Minimum-cost repair</a>
    <a href="#sec-result"><b>✔</b>Result</a>
    <a href="#sec-log"><b>≡</b>Activity log</a>
  </nav>
  <div class="side-foot">Console RBAC audit and repair engine</div>
</aside>
<div class="wrap">
<header>
  <h1>RBAC Audit Dashboard</h1>
  <label>Scenario <select id="scn" aria-label="Load a predefined scenario"></select></label>
  <button id="newBtn">New empty</button>
  <button id="openBtn">Open file…</button><input id="file" type="file" accept=".txt,.cfg,text/plain" class="sr">
  <button id="saveBtn">Save file</button>
  <span class="sp"></span>
  <button id="undoBtn" disabled>Undo</button>
</header>
<div id="banner" style="padding:0 24px"></div>
<div id="note" style="padding:0 24px"></div>
<div class="hero"><div><div><h2>Find over-privileged access. Fix it with the cheapest safe change.</h2><p>Detects forbidden, excessive, missing and conflicting permissions, then searches every combination of repairs (backtracking with pruning) and validates the cheapest one before anything is applied.</p></div><div class="steps" id="steps" aria-label="Pipeline status"></div></div></div>
<div id="stats" aria-live="polite"></div>
<main>
  <div id="left">
    <section class="card" id="sec-build"><h2>1. Build your configuration</h2>
      <div class="tabs" role="tablist" id="etabs"></div>
      <div id="editor"></div>
    </section>
  </div>
  <div id="right">
    <section class="card" id="sec-settings"><h2>Repair settings</h2><div id="options"></div></section>
    <section class="card" id="sec-compliance"><h2>2. Compliance</h2><div id="compliance" class="mute">Loading…</div></section>
    <section class="card" id="sec-matrix"><h2>Privilege matrix</h2><div id="matrix"></div></section>
    <section class="card" id="sec-violations"><h2>3. Violations <span id="vcount"></span></h2><div id="violations"></div></section>
    <section class="card" id="sec-repair"><h2>4. Minimum-cost repair</h2><div id="repair"></div></section>
    <section class="card" id="sec-result"><h2>5. Result and validation</h2><div id="result"></div></section>
    <section class="card" id="sec-log"><h2>Activity log</h2><div id="log" class="mute"></div></section>
  </div>
</main>
</div>

<script>
"use strict";
// ---------- tiny DOM helper (always uses text nodes, so user text can never inject HTML) ----------
function h(tag, props, ...kids) {
  const e = document.createElement(tag);
  for (const k in (props || {})) {
    const v = props[k];
    if (k === 'class') e.className = v;
    else if (k.startsWith('on')) e.addEventListener(k.slice(2), v);
    else if (k === 'value' || k === 'checked' || k === 'disabled' || k === 'selected') e[k] = v;
    else e.setAttribute(k, v);
  }
  for (const kid of kids.flat(Infinity)) if (kid != null && kid !== false) e.append(kid.nodeType ? kid : String(kid));
  return e;
}
const $ = s => document.querySelector(s);
// replaceChildren that flattens arrays and skips null/false (the raw method would print "null")
function put(el, ...kids) { el.replaceChildren(...kids.flat(Infinity).filter(k => k != null && k !== false)); }
const NAME_RE = /^[A-Za-z0-9._-]{1,40}$/;

// ---------- state ----------
let model = emptyModel();
let result = null, scenarios = [], history = [], logLines = [], seq = 0, timer = null;
let tab = 'perms';
const opts = { uniform: false, pruning: true, revoke: 1, grant: 1, removerole: 2, addrole: 2 };

function emptyModel() { return { permissions: [], sod: [], roles: [], users: [] }; }
function fromPolicy(p) {
  return {
    permissions: p.permissions.map(x => ({ id: x.id, description: x.description, risk: x.risk })),
    sod: p.sod.map(x => ({ name: x.name, permissions: [...x.permissions] })),
    roles: p.roles.map(r => ({ name: r.name, parents: [...r.parents], grants: [...r.grants], allowed: [...r.allowed],
                               forbidden: [...r.forbidden], required: [...r.required] })),
    users: p.users.map(u => ({ name: u.name, roles: [...u.roles], direct: [...u.direct] }))
  };
}
function clone(m) { return JSON.parse(JSON.stringify(m)); }

// ---------- model -> config text (same format as data/*.txt) ----------
function roleOrder(m) {                     // parents before children
  const done = new Set(), out = [], by = new Map(m.roles.map(r => [r.name, r]));
  let progress = true;
  while (out.length < m.roles.length && progress) {
    progress = false;
    for (const r of m.roles) {
      if (done.has(r.name)) continue;
      if (r.parents.every(p => done.has(p) || !by.has(p))) { done.add(r.name); out.push(r); progress = true; }
    }
  }
  return out.length === m.roles.length ? out : null;   // null = cycle
}
function toConfig(m) {
  const L = [], ok = new Set(m.permissions.map(p => p.id));
  const clean = s => (s || '').replace(/[#\r\n]/g, ' ').trim();
  for (const p of m.permissions) L.push(`permission ${p.id} risk=${p.risk} | ${clean(p.description)}`);
  for (const s of m.sod) { const ps = s.permissions.filter(x => ok.has(x)); if (ps.length >= 2) L.push(`sod ${s.name} ${ps.join(' ')}`); }
  const roles = roleOrder(m);
  if (!roles) throw new Error('The role inheritance contains a cycle.');
  for (const r of roles) {
    L.push(`role ${r.name}`);
    if (r.parents.length) L.push(`  inherits ${r.parents.join(' ')}`);
    for (const k of ['grants', 'allowed', 'forbidden', 'required']) if (r[k].length) L.push(`  ${k} ${r[k].join(' ')}`);
  }
  for (const u of m.users) {
    L.push(`user ${u.name}`);
    if (u.roles.length) L.push(`  roles ${u.roles.join(' ')}`);
    if (u.direct.length) L.push(`  direct ${u.direct.join(' ')}`);
  }
  return L.join('\n') + '\n';
}

// ---------- server calls ----------
async function post(path, body) {
  const r = await fetch(path, { method: 'POST', body, headers: { 'Content-Type': 'text/plain' } });
  return r.json();
}
function queryString() {
  return `?uniform=${opts.uniform ? 1 : 0}&pruning=${opts.pruning ? 1 : 0}&revoke=${opts.revoke}&grant=${opts.grant}&removerole=${opts.removerole}&addrole=${opts.addrole}`;
}
function scheduleRefresh() { clearTimeout(timer); timer = setTimeout(refresh, 120); }
async function refresh() {
  const mine = ++seq;
  let text;
  try { text = toConfig(model); } catch (e) { showError(e.message); return; }
  try {
    const res = await post('/api/analyze' + queryString(), text);
    if (mine !== seq) return;                       // a newer edit already replaced this request
    if (!res.ok) { showError(res.error); return; }
    result = res; showError(null); renderResults();
  } catch (e) { showError('Cannot reach the server: ' + e.message); }
}
function showError(msg) {
  $('#banner').replaceChildren(msg ? h('div', { class: 'box err', role: 'alert' }, 'Cannot analyse: ', msg) : '');
  if (msg) { result = null; $('#stats').replaceChildren(); $('#steps').replaceChildren(); }
}
async function loadConfigText(text, note) {
  const res = await post('/api/parse', text);
  if (!res.ok) { showError(res.error); return false; }
  pushHistory(); model = fromPolicy(res.policy); log(note); renderEditor(); await refresh(); return true;
}

function pushHistory() { history.push(clone(model)); if (history.length > 50) history.shift(); $('#undoBtn').disabled = false; }
function undo() {
  if (!history.length) return;
  model = history.pop(); $('#undoBtn').disabled = history.length === 0; log('Undo'); renderEditor(); refresh();
}
function log(msg) { logLines.unshift(new Date().toLocaleTimeString() + '  ' + msg); logLines = logLines.slice(0, 40); renderLog(); }
function renderLog() { $('#log').replaceChildren(...logLines.map(l => h('div', null, l))); if (!logLines.length) $('#log').textContent = 'Nothing yet.'; }

// ---------- editor ----------
const TABS = [['perms', 'Permissions'], ['roles', 'Roles'], ['users', 'Users'], ['sod', 'Separation of duties'], ['text', 'Text / import']];
function renderTabs() {
  $('#etabs').replaceChildren(...TABS.map(([id, label]) => {
    const n = { perms: model.permissions.length, roles: model.roles.length, users: model.users.length, sod: model.sod.length }[id];
    return h('button', { role: 'tab', 'aria-selected': String(tab === id), onclick: () => { tab = id; renderEditor(); } }, label + (n != null ? ` (${n})` : ''));
  }));
}
function renderEditor() {
  renderTabs();
  const box = $('#editor');
  box.replaceChildren(({ perms: permsTab, roles: rolesTab, users: usersTab, sod: sodTab, text: textTab })[tab]());
}
function changed() { renderTabs(); scheduleRefresh(); }

function permChips(list, onToggle, extraNote) {
  const sel = new Set(list);
  if (!model.permissions.length) return h('div', { class: 'mute' }, 'Add permissions first.');
  return h('div', { class: 'chips' }, model.permissions.map(p => {
    const c = h('span', { class: 'chip' + (sel.has(p.id) ? ' on' : ''), role: 'checkbox', tabindex: '0', 'aria-checked': String(sel.has(p.id)), title: p.description }, p.id);
    const flip = () => { onToggle(p.id); const on = c.classList.toggle('on'); c.setAttribute('aria-checked', String(on)); };
    c.addEventListener('click', flip);
    c.addEventListener('keydown', e => { if (e.key === ' ' || e.key === 'Enter') { e.preventDefault(); flip(); } });
    return c;
  }));
}
function toggle(arr, v) { const i = arr.indexOf(v); if (i >= 0) arr.splice(i, 1); else arr.push(v); changed(); }
function nameChips(names, list, onToggle) {
  const sel = new Set(list);
  if (!names.length) return h('div', { class: 'mute' }, 'None available yet.');
  return h('div', { class: 'chips' }, names.map(n => {
    const c = h('span', { class: 'chip' + (sel.has(n) ? ' on' : ''), role: 'checkbox', tabindex: '0', 'aria-checked': String(sel.has(n)) }, n);
    const flip = () => { const r = onToggle(n); if (r === false) return; const on = c.classList.toggle('on'); c.setAttribute('aria-checked', String(on)); };
    c.addEventListener('click', flip);
    c.addEventListener('keydown', e => { if (e.key === ' ' || e.key === 'Enter') { e.preventDefault(); flip(); } });
    return c;
  }));
}
function askName(label, existing) {
  const v = (prompt(label) || '').trim();
  if (!v) return null;
  if (!NAME_RE.test(v)) { alert('Names may contain letters, digits, dot, underscore and dash (max 40).'); return null; }
  if (existing.includes(v)) { alert(`'${v}' already exists.`); return null; }
  return v;
}

function permsTab() {
  const rows = model.permissions.map(p => h('tr', null,
    h('td', null, h('code', null, p.id)),
    h('td', null, h('input', { type: 'text', value: p.description, 'aria-label': 'Description of ' + p.id, size: 26, onchange: e => { p.description = e.target.value; scheduleRefresh(); } })),
    h('td', null, h('input', { type: 'number', min: 1, max: 10, value: p.risk, 'aria-label': 'Risk of ' + p.id, onchange: e => { p.risk = Math.max(1, Math.min(10, parseInt(e.target.value, 10) || 1)); e.target.value = p.risk; scheduleRefresh(); } })),
    h('td', null, h('button', { class: 'danger', 'aria-label': 'Delete ' + p.id, onclick: () => deletePermission(p.id) }, 'Delete'))));
  return h('div', null,
    h('p', { class: 'mute' }, 'Risk (1-10) is how costly it is to change a permission: revoking or granting it costs base × risk.'),
    h('table', null, h('thead', null, h('tr', null, h('th', null, 'Id'), h('th', null, 'Description'), h('th', null, 'Risk'), h('th'))), h('tbody', null, rows)),
    h('div', { class: 'row' }, h('button', { onclick: () => { const id = askName('New permission id (e.g. reports.view):', model.permissions.map(p => p.id)); if (id) { pushHistory(); model.permissions.push({ id, description: '', risk: 1 }); log('Added permission ' + id); renderEditor(); scheduleRefresh(); } } }, '+ Add permission')));
}
function deletePermission(id) {
  if (!confirm(`Delete permission '${id}' everywhere (roles, users, rules)?`)) return;
  pushHistory();
  model.permissions = model.permissions.filter(p => p.id !== id);
  for (const r of model.roles) for (const k of ['grants', 'allowed', 'forbidden', 'required']) r[k] = r[k].filter(x => x !== id);
  for (const u of model.users) u.direct = u.direct.filter(x => x !== id);
  for (const s of model.sod) s.permissions = s.permissions.filter(x => x !== id);
  log('Deleted permission ' + id); renderEditor(); scheduleRefresh();
}

function createsCycle(child, parent) {   // would "child inherits parent" close a loop?
  const by = new Map(model.roles.map(r => [r.name, r]));
  const seen = new Set(), stack = [parent];
  while (stack.length) {
    const n = stack.pop();
    if (n === child) return true;
    if (seen.has(n)) continue; seen.add(n);
    for (const p of (by.get(n) || { parents: [] }).parents) stack.push(p);
  }
  return false;
}
function rolesTab() {
  const cards = model.roles.map(r => {
    const others = model.roles.filter(o => o.name !== r.name).map(o => o.name);
    return h('div', { class: 'role' },
      h('div', { class: 'head' }, h('strong', null, 'Role ', r.name),
        h('button', { class: 'danger', onclick: () => deleteRole(r.name) }, 'Delete')),
      h('h3', null, 'Inherits from'),
      nameChips(others, r.parents, n => {
        if (!r.parents.includes(n) && createsCycle(r.name, n)) { alert(`Refused: '${r.name}' inheriting '${n}' would create a cycle.`); return false; }
        toggle(r.parents, n); renderEditor();
      }),
      ...[['grants', 'Grants (what holders receive)'], ['allowed', 'Allowed (the most they may hold)'], ['forbidden', 'Forbidden (must never hold)'], ['required', 'Required (must always hold)']]
        .flatMap(([k, label]) => [h('h3', null, label), permChips(r[k], p => toggle(r[k], p))]));
  });
  return h('div', null,
    h('p', { class: 'mute' }, 'A role bundles permissions it grants with the policy rules for its holders. A role also inherits everything from its parents.'),
    cards,
    h('div', { class: 'row' }, h('button', { onclick: () => { const n = askName('New role name:', model.roles.map(r => r.name)); if (n) { pushHistory(); model.roles.push({ name: n, parents: [], grants: [], allowed: [], forbidden: [], required: [] }); log('Added role ' + n); renderEditor(); scheduleRefresh(); } } }, '+ Add role')));
}
function deleteRole(name) {
  if (!confirm(`Delete role '${name}'? It is also removed from users and from role inheritance.`)) return;
  pushHistory();
  model.roles = model.roles.filter(r => r.name !== name);
  for (const r of model.roles) r.parents = r.parents.filter(p => p !== name);
  for (const u of model.users) u.roles = u.roles.filter(r => r !== name);
  log('Deleted role ' + name); renderEditor(); scheduleRefresh();
}

function usersTab() {
  const cards = model.users.map(u => h('div', { class: 'user' },
    h('div', { class: 'head' }, h('strong', null, 'User ', u.name), h('button', { class: 'danger', onclick: () => { if (confirm(`Delete user '${u.name}'?`)) { pushHistory(); model.users = model.users.filter(x => x !== u); log('Deleted user ' + u.name); renderEditor(); scheduleRefresh(); } } }, 'Delete')),
    h('h3', null, 'Roles'), nameChips(model.roles.map(r => r.name), u.roles, n => toggle(u.roles, n)),
    h('h3', null, 'Direct permissions (individual overrides)'), permChips(u.direct, p => toggle(u.direct, p))));
  return h('div', null, cards,
    h('div', { class: 'row' }, h('button', { onclick: () => { const n = askName('New user name:', model.users.map(u => u.name)); if (n) { pushHistory(); model.users.push({ name: n, roles: [], direct: [] }); log('Added user ' + n); renderEditor(); scheduleRefresh(); } } }, '+ Add user')));
}

function sodTab() {
  const cards = model.sod.map(s => h('div', { class: 'sod' },
    h('div', { class: 'head' }, h('strong', null, 'Rule ', s.name), h('button', { class: 'danger', onclick: () => { pushHistory(); model.sod = model.sod.filter(x => x !== s); log('Deleted rule ' + s.name); renderEditor(); scheduleRefresh(); } }, 'Delete')),
    h('p', { class: 'mute' }, 'No user may hold more than one of these.'),
    permChips(s.permissions, p => toggle(s.permissions, p)),
    s.permissions.length < 2 ? h('div', { class: 'box warn' }, 'Select at least two permissions; until then this rule is not active.') : null));
  return h('div', null, h('p', { class: 'mute' }, 'Separation of duties: for example one person must not both manage accounts and export audit data.'), cards,
    h('div', { class: 'row' }, h('button', { onclick: () => { const n = askName('New rule name (e.g. SOD-ADMIN-AUDIT):', model.sod.map(s => s.name)); if (n) { pushHistory(); model.sod.push({ name: n, permissions: [] }); log('Added SoD rule ' + n); renderEditor(); } } }, '+ Add rule')));
}

function textTab() {
  let text;
  try { text = toConfig(model); } catch (e) { text = '# ' + e.message; }
  const ta = h('textarea', { spellcheck: 'false', 'aria-label': 'Configuration text', value: text });
  return h('div', null,
    h('p', { class: 'mute' }, 'The same text format as the data/*.txt files. Edit or paste a configuration and apply it.'),
    ta, h('div', { class: 'row' }, h('button', { class: 'primary', onclick: () => loadConfigText(ta.value, 'Loaded configuration from text') }, 'Apply text'),
      h('button', { onclick: () => { ta.value = text; } }, 'Reset')));
}

// ---------- results ----------
const RATING_COLOR = { 'SECURE': 'var(--ok)', 'LOW RISK': 'var(--ok)', 'MODERATE RISK': 'var(--warn)', 'HIGH RISK': 'var(--bad)', 'CRITICAL RISK': 'var(--bad)' };
function svgHtml(markup) { const t = document.createElement('template'); t.innerHTML = markup.trim(); return t.content.firstChild; }
function gauge(label, c) {          // donut gauge; only numbers and fixed server strings go into the markup
  const R = 52, C = 2 * Math.PI * R, v = Math.max(0, Math.min(100, c.overall)), col = RATING_COLOR[c.rating] || 'var(--accent)';
  const svg = svgHtml(`<svg width="132" height="132" viewBox="0 0 132 132" role="img" aria-label="${label} ${v.toFixed(1)} of 100">
    <circle cx="66" cy="66" r="${R}" fill="none" stroke="var(--chip)" stroke-width="13"/>
    <circle class="v" cx="66" cy="66" r="${R}" fill="none" stroke="${col}" stroke-width="13" stroke-linecap="round"
      stroke-dasharray="${(C * v / 100).toFixed(1)} ${C.toFixed(1)}" transform="rotate(-90 66 66)"/>
    <text x="66" y="68" text-anchor="middle" font-size="26" font-weight="700" fill="var(--ink)">${v.toFixed(1)}</text>
    <text x="66" y="86" text-anchor="middle" font-size="11" fill="var(--mute)">of 100</text></svg>`);
  return h('div', { class: 'gauge' }, svg, h('div', { class: 'r', style: 'color:' + col }, c.rating), h('div', { class: 't' }, label));
}
const SEV_COLOR = { CRITICAL: '#d9453b', HIGH: '#e08a1e', MEDIUM: '#5b5fe3', LOW: '#9aa0c8' };
function severityChart(violations) {
  const counts = { CRITICAL: 0, HIGH: 0, MEDIUM: 0, LOW: 0 };
  for (const v of violations) counts[v.severity]++;
  const total = violations.length; if (!total) return null;
  return h('div', null,
    h('div', { class: 'stack', role: 'img', 'aria-label': 'Violations by severity' }, Object.keys(counts).filter(k => counts[k]).map(k => h('i', { style: `width:${100 * counts[k] / total}%;background:${SEV_COLOR[k]}`, title: `${k}: ${counts[k]}` }))),
    h('div', { class: 'legend' }, Object.keys(counts).map(k => h('span', null, h('b', { style: 'background:' + SEV_COLOR[k] }), `${k} ${counts[k]}`))));
}
function userBars(users) {
  return users.map(u => { const col = u.score >= 90 ? 'var(--ok)' : (u.score >= 70 ? 'var(--warn)' : 'var(--bad)');
    return h('div', { class: 'ubar' }, h('span', null, u.name), h('div', { class: 'bar' }, h('i', { style: `width:${u.score}%;background:${col}` })), h('strong', null, u.score)); });
}
function renderMatrix(p) {
  const el = $('#matrix');
  if (!p.users.length || !p.permissions.length) { el.replaceChildren(h('p', { class: 'mute' }, 'Add permissions and users to see who holds what.')); return; }
  const roles = new Map(p.roles.map(r => [r.name, r.resolved]));
  const rows = p.users.map(u => {
    const allowed = new Set(), forb = new Set(), req = new Set(), eff = new Set(u.effective.map(e => e.permission));
    for (const rn of u.roles) { const r = roles.get(rn); if (!r) continue; r.allowed.forEach(x => allowed.add(x)); r.forbidden.forEach(x => forb.add(x)); r.required.forEach(x => req.add(x)); }
    return h('tr', null, h('th', null, u.name), p.permissions.map(pm => {
      let sym = '', cls = '', tip = 'not held';
      if (eff.has(pm.id)) {
        if (forb.has(pm.id)) { sym = '✕'; cls = 'm-bad'; tip = 'held but FORBIDDEN'; }
        else if (!allowed.has(pm.id)) { sym = '▲'; cls = 'm-warn'; tip = 'held but not allowed (excessive)'; }
        else { sym = '●'; cls = 'm-ok'; tip = 'held and allowed'; }
      } else if (req.has(pm.id)) { sym = '○'; cls = 'm-miss'; tip = 'REQUIRED but missing'; }
      return h('td', { class: 'c ' + cls, title: `${u.name} / ${pm.id}: ${tip}`, 'aria-label': `${u.name} ${pm.id} ${tip}` }, sym);
    }));
  });
  put(el, h('div', { class: 'matrix' }, h('table', null,
      h('thead', null, h('tr', null, h('th'), p.permissions.map(pm => h('th', { class: 'pc' }, pm.id)))), h('tbody', null, rows))),
    h('div', { class: 'legend', style: 'margin-top:8px' }, h('span', null, h('b', { style: 'background:var(--accent)' }), '● held, allowed'), h('span', null, h('b', { style: 'background:#e08a1e' }), '▲ held, not allowed'),
      h('span', null, h('b', { style: 'background:#d9453b' }), '✕ held, forbidden'), h('span', null, h('b', { style: 'background:#d9453b;opacity:.45' }), '○ required, missing')),
    p.hierarchyTree && p.hierarchyTree.trim() ? [h('h3', null, 'Role hierarchy'), h('div', { class: 'tree' }, p.hierarchyTree)] : null);
}
function renderSteps(r) {
  const clean = r.violations.length === 0, plan = r.plan;
  const steps = [['Detect', 'done'], ['Plan', clean ? 'done' : (plan.solvable ? 'done' : 'fail')],
    ['Validate', clean ? 'done' : (r.repaired ? (r.repaired.validation.passed ? 'done' : 'fail') : 'fail')], ['Apply', clean ? 'done' : '']];
  $('#steps').replaceChildren(...steps.map(([n, st], i) => h('div', { class: 'step ' + st }, h('i', null, st === 'done' ? '✓' : (st === 'fail' ? '✕' : String(i + 1))), n)));
}
function meter(label, c) {
  return h('div', { class: 'score' }, h('div', { class: 'mute' }, label),
    h('div', { class: 'n' }, c.overall.toFixed(1), h('span', { class: 'mute', style: 'font-size:14px' }, ' / 100')),
    h('div', { class: 'bar', role: 'img', 'aria-label': `${label} ${c.overall.toFixed(1)} of 100` }, h('i', { style: `width:${c.overall}%;background:${RATING_COLOR[c.rating] || 'var(--accent)'}` })),
    h('strong', { style: `color:${RATING_COLOR[c.rating]}` }, c.rating));
}
function renderResults() {
  const r = result;
  // compliance
  const wrap = h('div', { class: 'gauges' }, gauge('Before repair', r.compliance));
  if (r.repaired && r.plan.perUser.length) wrap.append(h('div', { class: 'arrow', 'aria-hidden': 'true' }, '→'), gauge('After simulated repair', r.repaired.compliance));
  const perUser = r.compliance.users.length ? h('div', null, userBars(r.compliance.users)) : h('p', { class: 'mute' }, 'No users yet.');
  $('#compliance').replaceChildren(wrap, h('h3', null, 'Per user'), perUser);

  // violations
  $('#vcount').replaceChildren(h('span', { class: 'tag ' + (r.violations.length ? 'bad' : 'good') }, String(r.violations.length)));
  put($('#violations'), severityChart(r.violations), r.violations.length ?
    h('table', null, h('thead', null, h('tr', null, ['Severity', 'Type', 'Who', 'Permission', 'Why'].map(t => h('th', null, t)))),
      h('tbody', null, r.violations.map(v => h('tr', null,
        h('td', null, h('span', { class: 'tag ' + v.severity }, v.severity)), h('td', null, v.type),
        h('td', null, v.roleLevel ? 'role ' + v.role : v.user), h('td', null, v.permission ? h('code', null, v.permission) : ''), h('td', null, v.description))))) :
    h('div', { class: 'box good' }, 'No violations. This configuration complies with every rule.'),
    r.roleIssues ? h('p', { class: 'mute' }, `${r.roleIssues} role-definition advisories are shown above. The engine repairs users only, so role definitions are never changed automatically.`) : null);

  renderStats(); renderSteps(r); renderMatrix(r.policy); renderRepair(); renderOutcome();
}

function renderStats() {
  const r = result, plan = r.plan;
  const card = (sym, color, value, label) => h('div', { class: 'stat' },
    h('div', { class: 'ico', style: 'background:' + color, 'aria-hidden': 'true' }, sym),
    h('div', null, h('div', { class: 'v' }, value), h('div', { class: 'l' }, label)));
  const cost = !plan.perUser.length ? '0' : (plan.solvable ? String(plan.totalCost) : 'none');
  $('#stats').replaceChildren(
    card('◔', 'linear-gradient(135deg,#5b5fe3,#8a63f0)', r.compliance.overall.toFixed(1) + ' (' + r.compliance.rating + ')', 'Compliance score' + (r.repaired ? ', after repair ' + r.repaired.compliance.overall.toFixed(1) : '')),
    card('!', r.violations.length ? '#d9453b' : '#1f9d5c', String(r.userViolations), 'User violations' + (r.roleIssues ? ' (+' + r.roleIssues + ' role advisories)' : '')),
    card('✦', '#e08a1e', cost, 'Minimum repair cost'),
    card('☰', '#2a8bd6', r.policy.users.length + ' / ' + r.policy.roles.length + ' / ' + r.policy.permissions.length, 'Users / roles / permissions'));
}

// highlight the sidebar link of the section being viewed
(function () {
  const links = [...document.querySelectorAll('nav a')];
  links.forEach(a => a.addEventListener('click', () => { links.forEach(x => x.classList.toggle('on', x === a)); }));
  if (!('IntersectionObserver' in window)) return;
  const io = new IntersectionObserver(es => {
    for (const e of es) if (e.isIntersecting) links.forEach(a => a.classList.toggle('on', a.getAttribute('href') === '#' + e.target.id));
  }, { rootMargin: '-20% 0px -70% 0px' });
  document.querySelectorAll('section.card').forEach(sec => io.observe(sec));
})();

function renderRepair() {
  const r = result, plan = r.plan;
  if (!plan.perUser.length) { $('#repair').replaceChildren(h('p', { class: 'mute' }, 'Nothing to repair.')); return; }
  const head = plan.solvable
    ? h('div', { class: 'box good' }, `Minimum total cost: `, h('strong', null, plan.totalCost), `. Search: ${plan.totals.nodes} nodes visited, ${plan.totals.explored} combinations evaluated, ${plan.totals.pruned} branches pruned (${r.pruning ? 'backtracking with pruning' : 'pure brute force'}).`)
    : h('div', { class: 'box err' }, 'At least one user cannot be repaired within the allowed actions. Nothing is changed; see the reasons below.');
  const users = plan.perUser.map(u => {
    const alt = u.alternatives.filter(a => a.cost !== u.cost || JSON.stringify(a.actions.map(x => x.text)) !== JSON.stringify(u.best.map(x => x.text)));
    return h('details', { open: '' }, h('summary', null, `${u.user}: `, u.solvable ? `best repair costs ${u.cost}` : (u.tooLarge ? 'search too large' : 'no valid repair'),
        ` (${u.stats.generated} candidate actions, ${u.stats.searchSpace} combinations)`),
      u.solvable ? h('ol', null, u.best.map(a => h('li', null, h('span', { class: 'tag good' }, a.type), ' ', a.text))) : h('div', { class: 'box err' }, u.explanation || 'No valid combination exists.'),
      !u.solvable && u.rejections.length ? h('ul', null, u.rejections.map(x => h('li', null, `${x.count} × ${x.reason}`))) : null,
      h('h3', null, 'Candidate actions'),
      h('table', null, h('thead', null, h('tr', null, h('th', null, 'Action'), h('th', null, 'Cost'))),
        h('tbody', null, u.candidates.map(a => h('tr', null, h('td', null, a.text.replace(/ \(cost \d+\)$/, '')), h('td', null, a.cost))))),
      alt.length ? h('h3', null, 'Other valid solutions seen (more expensive or equal)') : null,
      alt.length ? h('ul', null, alt.slice(0, 6).map(a => h('li', null, `cost ${a.cost}: `, a.actions.map(x => x.type + ' ' + x.target).join(', ')))) : null,
      alt.length ? h('p', { class: 'mute' }, 'Pruned branches are not listed, so this list is incomplete when pruning is on.') : null);
  });
  put($('#repair'), head, users);
}

function diffTable(before, after) {
  const aMap = new Map(after.users.map(u => [u.name, u]));
  const rows = [];
  for (const b of before.users) {
    const a = aMap.get(b.name); if (!a) continue;
    const bs = new Set(b.effective.map(e => e.permission)), as = new Set(a.effective.map(e => e.permission));
    const all = [...new Set([...bs, ...as])].sort();
    const changed = all.some(p => bs.has(p) !== as.has(p)) || JSON.stringify(b.roles) !== JSON.stringify(a.roles);
    if (!changed) continue;
    rows.push(h('tr', null, h('td', null, b.name),
      h('td', null, b.roles.join(', ') || '-', h('br'), h('span', { class: 'mute' }, '→ '), a.roles.join(', ') || '-'),
      h('td', null, all.map(p => h('span', { class: bs.has(p) && as.has(p) ? 'chip static' : (as.has(p) ? 'chip static add' : 'chip static rem') }, (as.has(p) && !bs.has(p) ? '+ ' : (bs.has(p) && !as.has(p) ? '− ' : '')) + p)))));
  }
  return rows.length ? h('table', null, h('thead', null, h('tr', null, h('th', null, 'User'), h('th', null, 'Roles'), h('th', null, 'Effective permissions (+ gained, − lost)'))), h('tbody', null, rows)) : null;
}

function renderOutcome() {
  const r = result, box = $('#result');
  if (!r.repaired) {
    box.replaceChildren(r.plan.perUser.length ? h('p', { class: 'mute' }, 'No repair was simulated because at least one user is unrepairable.') : h('p', { class: 'mute' }, 'Nothing to validate.'));
    return;
  }
  const v = r.repaired.validation;
  const apply = h('button', { class: 'primary', disabled: !r.plan.perUser.length || !v.passed, onclick: applyRepair }, 'Apply repair to the configuration');
  const kids = [
    h('div', { class: 'box ' + (v.passed ? 'good' : 'err') }, v.passed ? `Validation PASSED. ${v.remainingViolations} violations remain after the simulated repair.` : 'Validation FAILED. The repair will not be applied.'),
    h('table', null, h('tbody', null, v.checks.map(c => h('tr', null, h('td', null, h('span', { class: 'tag ' + (c.passed ? 'good' : 'bad') }, c.passed ? 'PASS' : 'FAIL')), h('td', null, c.name, c.details ? h('div', { class: 'mute' }, c.details) : null)))))
  ];
  if (r.plan.perUser.length) {
    kids.push(h('h3', null, 'What changes'), diffTable(r.policy, r.repaired.policy) || h('p', { class: 'mute' }, 'No change.'));
    kids.push(h('p', { class: 'mute' }, 'The repair was simulated on a copy. Your configuration changes only when you press the button (Undo reverses it).'), h('div', { class: 'row' }, apply));
  }
  box.replaceChildren(...kids);
}
async function applyRepair() {
  const r = result; if (!r || !r.repaired || !r.repaired.validation.passed) return;
  const cost = r.plan.totalCost, n = r.repaired.actions.length;
  if (!confirm(`Apply ${n} repair action(s), total cost ${cost}?`)) return;
  const text = r.repaired.config;
  await loadConfigText(text, `Applied repair: ${n} action(s), cost ${cost}`);
  for (const a of r.repaired.actions) log('  ' + a.text);
}

// ---------- options ----------
function renderOptions() {
  const num = (key, label) => h('label', null, label + ' ', h('input', { type: 'number', min: 1, max: 100, value: opts[key], disabled: opts.uniform, onchange: e => { opts[key] = Math.max(1, parseInt(e.target.value, 10) || 1); scheduleRefresh(); } }));
  $('#options').replaceChildren(
    h('div', { class: 'row' },
      h('label', null, h('input', { type: 'checkbox', checked: opts.uniform, onchange: e => { opts.uniform = e.target.checked; renderOptions(); scheduleRefresh(); } }), ' Uniform cost (every change costs 1)'),
      h('label', null, h('input', { type: 'checkbox', checked: opts.pruning, onchange: e => { opts.pruning = e.target.checked; scheduleRefresh(); } }), ' Pruning (off = pure brute force)')),
    h('div', { class: 'row' }, num('revoke', 'Revoke ×risk'), num('grant', 'Grant ×risk'), num('removerole', 'Remove role'), num('addrole', 'Add role')));
}

// ---------- files, scenarios ----------
function download(name, text) {
  const a = h('a', { href: URL.createObjectURL(new Blob([text], { type: 'text/plain' })), download: name });
  document.body.append(a); a.click(); a.remove(); setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}
$('#saveBtn').onclick = () => { try { download('privilege-tetris-config.txt', toConfig(model)); log('Saved configuration to a file'); } catch (e) { alert(e.message); } };
$('#openBtn').onclick = () => $('#file').click();
$('#file').onchange = async e => { const f = e.target.files[0]; if (!f) return; if (f.size > 1 << 20) { alert('File too large (max 1 MB).'); return; } await loadConfigText(await f.text(), 'Opened file ' + f.name); e.target.value = ''; };
$('#newBtn').onclick = () => { if (!confirm('Start an empty configuration? (You can Undo.)')) return; pushHistory(); model = emptyModel(); $('#scn').value = ''; $('#note').replaceChildren(); log('New empty configuration'); renderEditor(); refresh(); };
$('#undoBtn').onclick = undo;
$('#scn').onchange = async e => {
  const s = scenarios.find(x => String(x.id) === e.target.value); if (!s) return;
  await loadConfigText(s.config, `Loaded scenario ${s.id}: ${s.title}`);
  const exp = s.expectedCost < 0 ? 'no valid repair exists' : `minimum repair cost ${s.expectedCost}`;
  $('#note').replaceChildren(h('div', { class: 'box note' }, h('strong', null, `Scenario ${s.id}: ${s.title}. `), s.description, ' ', h('em', null, `Expected: ${s.expectedViolations} violation(s), ${exp}.`)));
};

async function start() {
  renderOptions(); renderLog();
  try {
    const res = await (await fetch('/api/scenarios')).json();
    scenarios = res.scenarios;
    $('#scn').replaceChildren(h('option', { value: '' }, '— choose —'), ...scenarios.map(s => h('option', { value: s.id }, `${s.id}. ${s.title}`)));
    renderEditor();
    if (scenarios.length) { $('#scn').value = String(scenarios[3] ? scenarios[3].id : scenarios[0].id); $('#scn').dispatchEvent(new Event('change')); }
  } catch (e) { showError('Cannot reach the server: ' + e.message); }
}
start();
</script>
</body>
</html>
)WEBPAGE";
}
