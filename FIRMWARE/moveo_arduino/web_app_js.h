#pragma once

#include <Arduino.h>

/* Script shared by all pages, served at /app.js: connection state, API helper,
   toast, E-stop and status polling.
   Included only by web_server.cpp. */
static const char APP_JS[] PROGMEM = R"rawjs(
// Shared by all Moveo pages: connection state, API helper, toast, E-stop
// and status polling. A page may define onStatus(d) to render /status replies.

// ── Connection state ──────────────────────────────────────────────────────────
const dot  = document.getElementById('dot');
const csts = document.getElementById('conn-status');
let connected = false;
function setConnected(ok){
  connected = ok;
  dot.className  = 'conn-dot ' + (ok ? 'ok' : 'err');
  csts.textContent = ok ? 'Connected' : 'Disconnected';
}

// ── API helpers ───────────────────────────────────────────────────────────────
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
    if(!r.ok){ showToast(d.error || ('HTTP ' + r.status), 'err'); return null; }
    return d;
  } catch(e){
    clearTimeout(tid);
    const msg = e.name === 'AbortError' ? 'Request timed out' : e.message;
    showToast(msg, 'err');
    setConnected(false);
    return null;
  }
}

function showToast(msg, type='ok'){
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.className = 'toast show ' + type;
  clearTimeout(t._timer);
  t._timer = setTimeout(() => t.className='toast', 2800);
}

// ── Shared widgets & actions ──────────────────────────────────────────────────
const stepChips = id => [1,10,100,1000]
  .map(n => `<button class="chip" onclick="setSteps('${id}',${n})">${n}</button>`).join('');
function setSteps(id, n){ document.getElementById('steps-'+id).value = n; }

async function stopAll(){
  const r = await api('/stop', {});
  if(r){ setConnected(true); showToast('All motors stopped', 'err'); }
}

// ── Sequential status polling (no pile-up) ────────────────────────────────────
let polling = false;
async function pollStatus(){
  if(polling) return;
  polling = true;
  const ctrl = new AbortController();
  const tid  = setTimeout(() => ctrl.abort(), TIMEOUT_MS);
  try {
    const r = await fetch('/status', {signal: ctrl.signal});
    clearTimeout(tid);
    if(!r.ok) throw new Error();
    const d = await r.json();
    setConnected(true);
    if(typeof onStatus === 'function') onStatus(d);
  } catch(_){
    clearTimeout(tid);
    setConnected(false);
  } finally {
    polling = false;
  }
}

// Start polling once the page's own script has run; sequential guard prevents pile-up
window.addEventListener('load', () => {
  setInterval(pollStatus, 2000);
  pollStatus();
});
)rawjs";
