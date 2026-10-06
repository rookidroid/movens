#pragma once

#include <Arduino.h>

/* WiFi settings page, served at /network.
   Included only by web_server.cpp. */
static const char NETWORK_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Movens Network</title>
<link rel="icon" type="image/svg+xml" href="/favicon.svg"/>
<link rel="stylesheet" href="/app.css"/>
</head>
<body data-page="network">
<header class="appbar" id="appbar"></header>

<main class="layout">

  <div class="side">
  <!-- 01 · Current connection -->
  <section class="card">
    <div class="card-head">
      <span class="idx">01</span><h2>Connection</h2><span class="spacer"></span>
      <span class="badge" id="w-badge">—</span>
    </div>
    <div class="card-body">
      <div class="kv">
        <span class="k">Mode</span><span class="v" id="w-mode">—</span>
        <span class="k">Network</span><span class="v" id="w-ssid">—</span>
        <span class="k">IP address</span><span class="v" id="w-ip">—</span>
        <span class="k">Hostname</span><span class="v" id="w-host">—</span>
        <span class="k">Signal</span><span class="v" id="w-rssi">—</span>
      </div>
      <div class="notice" id="w-fallback" hidden>
        Couldn't join <b id="w-saved"></b> at power-on, so the robot started its own access point.
        Check the network name and password, then save again.
      </div>
      <p class="hint">At power-on the robot joins the saved network. If none is saved, or it can't connect, it
        starts its own access point <b class="w-ap">movens</b> at <b>192.168.4.1</b> instead.</p>
    </div>
  </section>
  </div>

  <!-- 02 · Network to join -->
  <section class="card">
    <div class="card-head"><span class="idx">02</span><h2>WiFi network</h2></div>
    <div class="card-body">
      <div class="sub">
        <span class="lbl">Available networks</span><span class="spacer"></span>
        <button type="button" class="btn sm" id="btnScan" onclick="scan()">Scan</button>
      </div>
      <div class="netlist" id="netlist"></div>

      <label class="field"><span>Network name (SSID)</span>
        <span class="inp"><input type="text" id="in-ssid" maxlength="32" autocomplete="off"
          autocapitalize="none" spellcheck="false" oninput="markSel()"/></span></label>
      <label class="field"><span>Password</span>
        <span class="inp"><input type="password" id="in-pass" maxlength="64" autocomplete="off"
          placeholder="Empty for an open network"/></span></label>
      <label class="check"><input type="checkbox" onchange="$('in-pass').type = this.checked ? 'text' : 'password'"/>
        Show password</label>

      <div class="actions">
        <button type="button" class="btn ghost" id="btnForget">Forget network</button>
        <span class="grow"></span>
        <button type="button" class="btn primary" id="btnSave">Save &amp; restart</button>
      </div>
      <div class="notice" id="w-next" hidden></div>
      <p class="hint">Saving stops the motors and restarts the robot. Once it has joined your network, connect
        this device to the same network and open <b id="w-url">http://movens.local</b> or the IP address your
        router assigned. If it can't join, reconnect to the <b class="w-ap">movens</b> access point.</p>
    </div>
  </section>
</main>

<div class="toast" id="toast" role="status" aria-live="polite"></div>

<script src="/app.js"></script>
<script>
let NETS = [];        // last scan, one entry per SSID, strongest first
let restarting = false;
let AP = 'movens';      // access point name

const esc = s => s.replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'})[c]);
const sleep = ms => new Promise(r => setTimeout(r, ms));

// Quiet GET for background refreshes: no error toasts while the robot restarts
async function peek(path){
  try { const r = await fetch(path); return r.ok ? await r.json() : null; } catch(_){ return null; }
}

function renderWifi(d){
  const sta = d.mode === 'sta';
  $('w-mode').textContent = sta ? 'Station' : 'Access point';
  $('w-ssid').textContent = d.ssid || '—';
  $('w-ip').textContent   = d.ip;
  $('w-host').textContent = d.host;
  $('w-rssi').textContent = sta && d.rssi != null ? `${d.rssi} dBm` : '—';
  $('w-badge').textContent = sta ? (d.connected ? 'Joined' : 'Reconnecting') : 'Access point';
  $('w-badge').className   = 'badge ' + (sta ? (d.connected ? 'ok' : 'warn') : '');
  $('w-fallback').hidden  = sta || !d.saved;
  $('w-saved').textContent = d.saved;
  $('w-url').textContent  = 'http://' + d.host;
  AP = d.ap;
  document.querySelectorAll('.w-ap').forEach(e => e.textContent = AP);
  $('btnForget').disabled = !d.saved;
}

async function loadWifi(fill){
  const d = fill ? await api('/wifi') : await peek('/wifi');
  if(!d) return;
  renderWifi(d);
  if(fill){ $('in-ssid').value = d.saved; markSel(); }
}

// ── Scan ──────────────────────────────────────────────────────────────────────
function bars(rssi){
  const n = rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : 1;
  return `<span class="bars" title="${rssi} dBm">${[1, 2, 3, 4].map(i => `<i${i <= n ? ' class="on"' : ''}></i>`).join('')}</span>`;
}

function renderNets(list){
  const best = {};
  for(const n of list) if(!best[n.ssid] || n.rssi > best[n.ssid].rssi) best[n.ssid] = n;
  NETS = Object.values(best).sort((a, b) => b.rssi - a.rssi);
  $('netlist').innerHTML = NETS.length
    ? NETS.map((n, i) => `
      <button type="button" class="net" onclick="pickNet(${i})">
        <span class="nm">${esc(n.ssid)}</span><span class="lk">${n.secure ? 'WPA' : 'OPEN'}</span>
        ${bars(n.rssi)}<span class="db">${n.rssi}</span>
      </button>`).join('')
    : '<p class="hint" style="padding:8px 10px">No networks found</p>';
  markSel();
}

function pickNet(i){
  const n = NETS[i];
  $('in-ssid').value = n.ssid;
  $('in-pass').value = '';
  markSel();
  if(n.secure) $('in-pass').focus();
}

// Highlight the scanned network matching the SSID field
function markSel(){
  const ssid = $('in-ssid').value;
  document.querySelectorAll('.net').forEach((b, i) => b.setAttribute('aria-pressed', NETS[i]?.ssid === ssid));
}

async function scan(){
  const b = $('btnScan');
  b.disabled = true;
  b.textContent = 'Scanning…';
  try {
    for(let i = 0; i < 30; i++){
      const d = await api('/wifiscan');
      if(!d) return;
      if(d.networks){ renderNets(d.networks); return; }
      await sleep(500);
    }
    showToast('Scan timed out', 'err');
  } finally {
    b.disabled = false;
    b.textContent = 'Scan';
  }
}

// ── Save / forget ─────────────────────────────────────────────────────────────
function showNext(html){
  restarting = true;
  $('w-next').innerHTML = html;
  $('w-next').hidden = false;
}

async function save(){
  const ssid = $('in-ssid').value, password = $('in-pass').value;
  if(!ssid) return showToast('Enter a network name', 'err');
  if(new TextEncoder().encode(ssid).length > 32) return showToast('Network name is longer than 32 bytes', 'err');
  if(password && (password.length < 8 || password.length > 64))
    return showToast('Password must be 8–64 characters, or empty for an open network', 'err');
  if(!await api('/wifi', {ssid, password})) return;
  showToast('Saved — restarting');
  showNext(`Restarting and joining <b>${esc(ssid)}</b>. Connect this device to that network, then open
    <b>${esc($('w-url').textContent)}</b> or the robot's IP address from your router.`);
}

async function forget(){
  if(!await api('/wifi', {ssid: ''})) return;
  showToast('Network forgotten — restarting');
  showNext(`Restarting as access point <b>${esc(AP)}</b>. Connect this device to it, then
    open <b>http://192.168.4.1</b>.`);
}

confirmTap($('btnSave'), save, 'Tap again to restart');
confirmTap($('btnForget'), forget, 'Tap again to forget');

// Refresh signal strength every few seconds (paused once a restart is under way)
setInterval(() => { if(!document.hidden && !restarting) loadWifi(false); }, 5000);
loadWifi(true);
</script>
</body>
</html>
)rawhtml";
