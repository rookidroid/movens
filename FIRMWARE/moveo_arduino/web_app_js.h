#pragma once

#include <Arduino.h>

/* Script shared by all pages, served at /app.js: app bar, connection state,
   API helper, toast, E-stop, status polling and small UI widgets.
   Included only by web_server.cpp. */
static const char APP_JS[] PROGMEM = R"rawjs(
// Shared by all Moveo pages. A page sets <body data-page="..."> with an empty
// <header id="appbar"> and <div id="toast">, and may define onStatus(d) to
// render /status replies.

const $ = id => document.getElementById(id);
const JOINT_NAMES = ['Base', 'Shoulder', 'Elbow', 'Wrist roll', 'Wrist pitch'];
const isCal = c => !!c && Math.abs(c.spd) > 1e-6;

// Signed fixed-point readout: +012.3 style, with a real minus sign
function fmt(v, d = 1){
  if(v == null || !Number.isFinite(v)) return '—';
  const s = Math.abs(v).toFixed(d);
  return (+v.toFixed(d) < 0 ? '−' : '+') + s;
}

// ── Per-browser UI preferences (never robot state) ───────────────────────────
function pref(key, def){
  try { const v = localStorage.getItem('moveo.' + key); return v == null ? def : JSON.parse(v); }
  catch(_){ return def; }
}
function setPref(key, v){
  try { localStorage.setItem('moveo.' + key, JSON.stringify(v)); } catch(_){}
}

// ── App bar (shared by every page) ────────────────────────────────────────────
(function buildAppBar(){
  const page = document.body.dataset.page;
  const link = (href, id, text) =>
    `<a href="${href}"${page === id ? ' class="active" aria-current="page"' : ''}>${text}</a>`;
  $('appbar').innerHTML = `
    <div class="brand"><span class="brand-mark">MOVEO</span><span class="brand-sub">5-AXIS ARM CONTROLLER</span></div>
    <nav class="nav">${link('/', 'control', 'Control')}${link('/calibrate', 'calibrate', 'Calibrate')}</nav>
    <div class="conn" title="Robot connection">
      <span class="led" id="dot"></span><span id="conn-status">Connecting</span>
      <span class="conn-host">${location.host || '192.168.4.1'}</span>
    </div>
    <button class="estop" id="btnStop" title="Stop all motors (Esc)" onclick="stopAll()">STOP <kbd>ESC</kbd></button>`;
})();

// ── Connection state ──────────────────────────────────────────────────────────
let connected = null;
function setConnected(ok){
  if(ok === connected) return;
  connected = ok;
  $('dot').className = 'led ' + (ok ? 'ok' : 'err');
  $('conn-status').textContent = ok ? 'Online' : 'Offline';
}

// ── API helper ────────────────────────────────────────────────────────────────
// GET without body, POST JSON with one. Returns the reply, or null after
// showing the error (also kept in api.lastError). Commands speed up polling.
const TIMEOUT_MS = 8000;

async function api(path, body){
  const ctrl = new AbortController();
  const tid  = setTimeout(() => ctrl.abort(), TIMEOUT_MS);
  try {
    const opts = body !== undefined
      ? {method:'POST', headers:{'Content-Type':'application/json'},
         body:JSON.stringify(body), signal:ctrl.signal}
      : {method:'GET', signal:ctrl.signal};
    const r = await fetch(path, opts);
    clearTimeout(tid);
    const d = await r.json().catch(() => ({}));
    setConnected(true);
    if(!r.ok){
      api.lastError = d.error || ('HTTP ' + r.status);
      showToast(api.lastError, 'err');
      return null;
    }
    if(body !== undefined) kick();
    return d;
  } catch(e){
    clearTimeout(tid);
    api.lastError = e.name === 'AbortError' ? 'Request timed out' : e.message;
    showToast(api.lastError, 'err');
    setConnected(false);
    return null;
  }
}

function showToast(msg, type = 'ok'){
  const t = $('toast');
  t.textContent = msg;
  t.className = 'toast show ' + type;
  clearTimeout(t._timer);
  t._timer = setTimeout(() => t.className = 'toast', 2800);
}

// ── E-stop: button in the app bar, or Esc anywhere ───────────────────────────
async function stopAll(){
  const b = $('btnStop');
  b.classList.remove('flash'); void b.offsetWidth; b.classList.add('flash');
  if(await api('/stop', {})) showToast('All motors stopped', 'err');
}
document.addEventListener('keydown', e => {
  if(e.key === 'Escape' && !e.repeat) stopAll();
});

// ── Widgets ───────────────────────────────────────────────────────────────────
// Segmented control. opts: values or [value, label] pairs. Returns
// {value, set(opts, value)}; onChange(value) fires on user clicks.
function seg(el, opts, value, onChange){
  const s = {value};
  s.set = (o, v) => {
    opts = o.map(x => Array.isArray(x) ? x : [x, String(x)]);
    s.value = opts.some(([x]) => x === v) ? v : opts[0][0];
    el.innerHTML = opts.map(([x, l], i) =>
      `<button type="button" role="radio" data-i="${i}" aria-checked="${x === s.value}">${l}</button>`).join('');
  };
  el.classList.add('seg');
  el.setAttribute('role', 'radiogroup');
  el.addEventListener('click', e => {
    const b = e.target.closest('button');
    if(!b) return;
    s.value = opts[b.dataset.i][0];
    el.querySelectorAll('button').forEach(x => x.setAttribute('aria-checked', x === b));
    if(onChange) onChange(s.value);
  });
  s.set(opts, value);
  return s;
}

// Two-tap confirmation: the first tap arms the button for 3 s, the second runs fn
function confirmTap(btn, fn, prompt = 'Tap again to confirm'){
  const label = btn.innerHTML;
  const reset = () => { delete btn.dataset.armed; btn.classList.remove('armed'); btn.innerHTML = label; };
  btn.addEventListener('click', () => {
    clearTimeout(btn._t);
    if(btn.dataset.armed){ reset(); fn(); return; }
    btn.dataset.armed = 1;
    btn.classList.add('armed');
    btn.textContent = prompt;
    btn._t = setTimeout(reset, 3000);
  });
}

// ── Adaptive status polling ──────────────────────────────────────────────────
// Fast while a joint moves or right after a command, slow when idle, paused
// while the tab is hidden. Sequential, so requests never pile up.
const POLL_FAST = 400, POLL_IDLE = 1500, BOOST_MS = 3000;
let polling = false, pollTimer = 0, boostUntil = 0, anyMoving = false;

function schedulePoll(ms){ clearTimeout(pollTimer); pollTimer = setTimeout(pollStatus, ms); }
function kick(){ boostUntil = Date.now() + BOOST_MS; if(!polling) schedulePoll(120); }

async function pollStatus(){
  if(polling || document.hidden) return;
  polling = true;
  let ok = false;
  const ctrl = new AbortController();
  const tid  = setTimeout(() => ctrl.abort(), TIMEOUT_MS);
  try {
    const r = await fetch('/status', {signal: ctrl.signal});
    clearTimeout(tid);
    if(!r.ok) throw new Error();
    const d = await r.json();
    ok = true;
    setConnected(true);
    anyMoving = [1, 2, 3, 4, 5].some(i => d['m' + i]);
    if(typeof onStatus === 'function') onStatus(d);
  } catch(_){
    clearTimeout(tid);
    setConnected(false);
  } finally {
    polling = false;
    schedulePoll(ok && (anyMoving || Date.now() < boostUntil) ? POLL_FAST : POLL_IDLE);
  }
}

document.addEventListener('visibilitychange', () => { if(!document.hidden) schedulePoll(0); });
// Start once the page's own script has run
window.addEventListener('load', () => pollStatus());
)rawjs";
