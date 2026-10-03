#pragma once

#include <Arduino.h>

/* Control page, served at /.
   Included only by web_server.cpp. */
static const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Moveo Control</title>
<link rel="stylesheet" href="/app.css"/>
</head>
<body data-page="control">
<header class="appbar" id="appbar"></header>

<main class="layout control">

  <div class="side">
  <!-- 01 · Arm state -->
  <section class="card area-state">
    <div class="card-head">
      <span class="idx">01</span><h2>Arm state</h2><span class="spacer"></span>
      <button class="btn sm" id="btnOrigin" title="Move every joint to its origin (0 steps)">&#8853; Go to origin</button>
    </div>
    <div class="card-body">
      <div class="pose">
        <div class="tile"><span class="k"><b class="ax-x">X</b>mm</span><span class="v" id="p-x">—</span></div>
        <div class="tile"><span class="k"><b class="ax-y">Y</b>mm</span><span class="v" id="p-y">—</span></div>
        <div class="tile"><span class="k"><b class="ax-z">Z</b>mm</span><span class="v" id="p-z">—</span></div>
        <div class="tile"><span class="k"><b class="ax-p">PITCH</b>&deg;</span><span class="v" id="p-pitch">—</span></div>
        <div class="tile"><span class="k"><b class="ax-p">YAW</b>&deg;</span><span class="v" id="p-yaw">—</span></div>
      </div>
      <div class="notice" id="pose-off" hidden>
        Tool position is unknown until J1&ndash;J5 are calibrated. <a href="/calibrate">Open calibration &rarr;</a>
      </div>
      <div class="jlist" id="jlist"></div>
    </div>
  </section>

  <!-- 03 · Gripper -->
  <section class="card area-grip">
    <div class="card-head">
      <span class="idx">03</span><h2>Gripper &middot; J6 servo</h2><span class="spacer"></span>
      <span class="big" id="servo-val">1500</span><span class="unit">&micro;s</span>
    </div>
    <div class="card-body">
      <div>
        <input type="range" id="servo" min="700" max="2300" step="10" value="1500" aria-label="Gripper pulse width"/>
        <div class="scale"><span>700</span><span>1100</span><span>1500</span><span>1900</span><span>2300</span></div>
      </div>
      <div class="actions fill">
        <button class="btn" onclick="sendServo(700)">Min</button>
        <button class="btn ghost" onclick="nudgeServo(-50)">&minus;50</button>
        <button class="btn" onclick="sendServo(1500)">Center</button>
        <button class="btn ghost" onclick="nudgeServo(50)">+50</button>
        <button class="btn" onclick="sendServo(2300)">Max</button>
      </div>
    </div>
  </section>
  </div>

  <!-- 02 · Motion control -->
  <section class="card area-ctrl">
    <div class="card-head"><span class="idx">02</span><h2>Motion control</h2></div>
    <div class="tabs" role="tablist">
      <button role="tab" data-tab="tool" onclick="showTab('tool')">Tool &middot; XYZ</button>
      <button role="tab" data-tab="joints" onclick="showTab('joints')">Joints</button>
    </div>

    <div class="tabpanel" id="tab-tool" role="tabpanel">
      <div class="notice" id="tool-lock" hidden>
        Tool (XYZ) control needs all five joints calibrated. Use the <b>Joints</b> tab meanwhile, or
        <a href="/calibrate">calibrate &rarr;</a>
      </div>
      <fieldset class="fs" id="tool-fs">
        <div class="sub"><h3>Jog tool</h3><span class="spacer"></span>
          <span class="lbl">Step</span><div id="seg-cart"></div></div>
        <div class="jogpad">
          <div class="padg">
            <span class="lbl">Plane &middot; top view</span>
            <div class="dpad">
              <span></span>
              <button type="button" class="jbtn" onclick="cartJog('x', 1)"><span class="ax-x">+X</span><small>Fwd</small></button>
              <span></span>
              <button type="button" class="jbtn" onclick="cartJog('y', 1)"><span class="ax-y">+Y</span><small>Left</small></button>
              <div class="dpad-c"><span id="dpad-step">10</span><small>mm&middot;&deg;</small></div>
              <button type="button" class="jbtn" onclick="cartJog('y',-1)"><span class="ax-y">&minus;Y</span><small>Right</small></button>
              <span></span>
              <button type="button" class="jbtn" onclick="cartJog('x',-1)"><span class="ax-x">&minus;X</span><small>Back</small></button>
              <span></span>
            </div>
          </div>
          <div class="padg">
            <span class="lbl">Height</span>
            <div class="vpair">
              <button type="button" class="jbtn" onclick="cartJog('z', 1)"><span class="ax-z">+Z</span><small>Up</small></button>
              <button type="button" class="jbtn" onclick="cartJog('z',-1)"><span class="ax-z">&minus;Z</span><small>Down</small></button>
            </div>
          </div>
          <div class="padg">
            <span class="lbl">Pitch</span>
            <div class="vpair">
              <button type="button" class="jbtn" onclick="cartJog('pitch', 1)"><span class="ax-p">+P</span><small>Tip fwd</small></button>
              <button type="button" class="jbtn" onclick="cartJog('pitch',-1)"><span class="ax-p">&minus;P</span><small>Tip back</small></button>
            </div>
          </div>
        </div>

        <hr class="rule"/>

        <div class="sub"><h3>Go to pose</h3></div>
        <form class="pform" id="cart-form" onsubmit="event.preventDefault(); cartGo(false)">
          <label class="field"><span class="ax-x">X</span><span class="inp"><input type="number" id="cart-x" value="300" step="any"/><i>mm</i></span></label>
          <label class="field"><span class="ax-y">Y</span><span class="inp"><input type="number" id="cart-y" value="0" step="any"/><i>mm</i></span></label>
          <label class="field"><span class="ax-z">Z</span><span class="inp"><input type="number" id="cart-z" value="200" step="any"/><i>mm</i></span></label>
          <label class="field"><span>Pitch</span><span class="inp"><input type="number" id="cart-pitch" value="-90" step="any"/><i>&deg;</i></span></label>
          <label class="field"><span>Yaw</span><span class="inp"><input type="number" id="cart-yaw" placeholder="auto" step="any"/><i>&deg;</i></span></label>
        </form>
        <p class="hint">Pitch &minus;90&deg; points the tool straight down. Leave yaw empty to keep the approach in the
          arm's vertical plane.</p>
        <div class="actions">
          <button type="button" class="btn ghost" onclick="cartFill()">Use current</button>
          <span class="grow"></span>
          <button type="button" class="btn" onclick="cartGo(true)">Check reach</button>
          <button type="submit" class="btn primary" form="cart-form">Move &#9656;</button>
        </div>
        <div class="result" id="cart-sol"></div>
      </fieldset>
    </div>

    <div class="tabpanel" id="tab-joints" role="tabpanel" hidden>
      <div class="toolbar">
        <div class="grp"><span class="lbl">Unit</span><div id="seg-unit"></div></div>
        <div class="grp"><span class="lbl">Step</span><div id="seg-jstep"></div></div>
      </div>
      <div class="jctl" id="jctl"></div>
      <details>
        <summary>Motion settings</summary>
        <div class="dbody">
          <table class="tbl">
            <thead><tr><th>Joint</th><th>Speed <span class="unit">steps/s</span></th><th>Accel <span class="unit">steps/s&sup2;</span></th></tr></thead>
            <tbody id="cfg-body"></tbody>
          </table>
          <p class="hint">Changed values are outlined. Settings last until the robot restarts.</p>
          <div class="actions"><span class="grow"></span>
            <button class="btn ghost" onclick="loadConfig()">Revert</button>
            <button class="btn primary" onclick="applyConfig()">Apply changes</button>
          </div>
        </div>
      </details>
    </div>
  </section>

</main>

<div class="toast" id="toast" role="status" aria-live="polite"></div>

<script src="/app.js"></script>
<script>
const N = JOINT_NAMES.length;
let CAL = {};          // /calib reply, keyed 'j1'..'j5'
let CFG = {};          // /config reply
let ST = {};           // last /status reply
let lastPose = null;   // tool pose from the last /status, null if any joint is uncalibrated
const cal = i => CAL['j' + i];

// ── Arm state ─────────────────────────────────────────────────────────────────
$('jlist').innerHTML = JOINT_NAMES.map((n, k) => { const i = k + 1; return `
  <div class="jrow">
    <span class="led" id="led-${i}"></span><span class="jtag">J${i}</span><span class="jname">${n}</span>
    <span class="jang" id="ang-${i}">—</span><span class="jstp" id="stp-${i}">—</span>
    <div class="meter" id="mtr-${i}"></div>
  </div>`; }).join('');

// Position of angle a on the joint's min..max scale, in %
const pct = (c, a) => (a - c.min) / (c.max - c.min) * 100;

function renderMeters(){
  for(let i = 1; i <= N; i++){
    const c = cal(i), el = $('mtr-' + i);
    if(!isCal(c)){
      el.className = 'meter off';
      el.innerHTML = 'Not calibrated &middot; <a href="/calibrate">calibrate</a>';
      continue;
    }
    const z = pct(c, 0);
    el.className = 'meter' + (c.limits ? ' lim' : '');
    el.title = c.limits ? 'Soft limits enforced' : 'Soft limits off';
    el.innerHTML = `<div class="trk"></div>` +
      (z >= 0 && z <= 100 ? `<i class="z" style="left:${z}%"></i>` : '') +
      `<i class="m" id="mk-${i}" hidden></i>` +
      `<span class="lo">${fmt(c.min, 0)}&deg;</span><span class="hi">${fmt(c.max, 0)}&deg;</span>`;
  }
}

// ── Joints tab ────────────────────────────────────────────────────────────────
const STEP_OPTS = {deg: [0.5, 1, 5, 10, 45], steps: [1, 10, 100, 1000]};
const jStep = {deg: pref('stepDeg', 5), steps: pref('stepSteps', 100)};
let unit = pref('unit', 'deg');

const segUnit = seg($('seg-unit'), [['deg', 'DEG'], ['steps', 'STEPS']], unit, v => {
  unit = v; setPref('unit', v);
  segJStep.set(STEP_OPTS[v], jStep[v]);
  renderJointCtl();
});
const segJStep = seg($('seg-jstep'), STEP_OPTS[unit], jStep[unit], v => {
  jStep[unit] = v; setPref(unit === 'deg' ? 'stepDeg' : 'stepSteps', v);
});

// 'deg', 'steps', or null when degrees are selected but the joint is uncalibrated
const jointUnit = i => unit === 'steps' ? 'steps' : (isCal(cal(i)) ? 'deg' : null);

$('jctl').innerHTML = JOINT_NAMES.map((n, k) => { const i = k + 1; return `
  <div class="jc" id="jc-${i}">
    <div class="jc-id">
      <div class="top"><span class="jtag">J${i}</span><span class="jname">${n}</span></div>
      <span class="jc-v" id="jcv-${i}">—</span>
    </div>
    <button class="btn jog" onclick="jointJog(${i},-1)" aria-label="Jog J${i} negative">&minus;</button>
    <button class="btn jog" onclick="jointJog(${i}, 1)" aria-label="Jog J${i} positive">+</button>
    <span class="inp"><input type="number" step="any" id="tgt-${i}" aria-label="J${i} target"
      onkeydown="if(event.key==='Enter') jointGo(${i})"/><i id="tu-${i}">&deg;</i></span>
    <button class="btn" onclick="jointGo(${i})">Go</button>
  </div>`; }).join('');

function renderJointCtl(){
  for(let i = 1; i <= N; i++){
    const u = jointUnit(i), row = $('jc-' + i);
    row.classList.toggle('nocal', !u);
    row.querySelectorAll('button,input').forEach(e => e.disabled = !u);
    $('tu-' + i).textContent = u === 'steps' ? 'st' : '°';
    $('tgt-' + i).placeholder = u ? 'target' : '';
  }
  renderJointValues();
}

function renderJointValues(){
  for(let i = 1; i <= N; i++){
    const u = jointUnit(i);
    $('jcv-' + i).textContent =
      !u ? 'Not calibrated — use steps' :
      u === 'deg' ? fmt(ST['a' + i]) + '°' : (ST['j' + i] ?? '—') + ' st';
  }
}

async function jointJog(i, dir){
  const u = jointUnit(i), step = segJStep.value;
  const steps = (u === 'deg' ? Math.round(step * cal(i).spd) : step) * dir;
  if(!steps) return showToast(`Step too small for J${i}`, 'err');
  await api('/move', {joint: i, steps});
}

async function jointGo(i){
  const u = jointUnit(i), v = parseFloat($('tgt-' + i).value);
  if(!Number.isFinite(v)) return showToast(`Enter a J${i} target`, 'err');
  const r = u === 'deg'
    ? await api('/moveangle', {joint: i, deg: v})
    : await api('/moveto', {joint: i, pos: Math.round(v)});
  if(r) showToast(`J${i} → ` + (u === 'deg' ? fmt(v) + '°' : Math.round(v) + ' st'));
}

// ── Motion settings ───────────────────────────────────────────────────────────
$('cfg-body').innerHTML = JOINT_NAMES.map((n, k) => { const i = k + 1; return `
  <tr><td>J${i} <span class="muted">${n}</span></td>
    <td><input type="number" min="1" id="spd-${i}" oninput="markCfg(${i})"/></td>
    <td><input type="number" min="1" id="acc-${i}" oninput="markCfg(${i})"/></td></tr>`; }).join('');

async function loadConfig(){
  const r = await api('/config');
  if(!r) return;
  CFG = r;
  for(let i = 1; i <= N; i++){
    const c = CFG['j' + i];
    if(!c) continue;
    $('spd-' + i).value = c.speed;
    $('acc-' + i).value = c.accel;
    markCfg(i);
  }
}

// [speed changed, accel changed]
function cfgChanged(i){
  const c = CFG['j' + i] || {};
  return [parseInt($('spd-' + i).value) !== c.speed, parseInt($('acc-' + i).value) !== c.accel];
}
function markCfg(i){
  const [s, a] = cfgChanged(i);
  $('spd-' + i).classList.toggle('dirty', s);
  $('acc-' + i).classList.toggle('dirty', a);
}

async function applyConfig(){
  let n = 0;
  for(let i = 1; i <= N; i++){
    if(!cfgChanged(i).some(Boolean)) continue;
    const speed = parseInt($('spd-' + i).value), accel = parseInt($('acc-' + i).value);
    if(!(speed > 0 && accel > 0)) return showToast(`J${i}: speed and accel must be positive`, 'err');
    if(!await api('/config', {joint: i, speed, accel})) return;
    n++;
  }
  if(!n) return showToast('No changes to apply');
  showToast(`Motion settings applied to ${n} joint${n > 1 ? 's' : ''}`);
  setTimeout(loadConfig, 300);  // the firmware applies them from loop()
}

// ── Tool (Cartesian / IK) ─────────────────────────────────────────────────────
const segCart = seg($('seg-cart'), [1, 5, 10, 50], pref('cartStep', 10), v => {
  setPref('cartStep', v); $('dpad-step').textContent = v;
});
$('dpad-step').textContent = segCart.value;

const fmtJoints = deg => deg.map((v, i) => `J${i + 1} ${fmt(v)}°`).join('  ');
function setResult(text, err){
  const el = $('cart-sol');
  el.textContent = text;
  el.className = 'result' + (err ? ' err' : '');
}

async function cartGo(dry){
  const body = {};
  for(const k of ['x', 'y', 'z', 'pitch', 'yaw']){
    const v = parseFloat($('cart-' + k).value);
    if(Number.isFinite(v)) body[k] = v;
  }
  if(!['x', 'y', 'z', 'pitch'].every(k => k in body)) return setResult('Enter X, Y, Z and pitch', true);
  if(dry) body.dry = 1;
  const r = await api('/movepose', body);
  if(!r) return setResult('✕ ' + api.lastError, true);
  setResult((dry ? 'REACHABLE ▸ ' : 'MOVING ▸ ') + fmtJoints(r.deg));
  if(!dry) showToast(`Tool → X ${body.x}  Y ${body.y}  Z ${body.z}`);
}

function cartFill(){
  if(!lastPose) return showToast('Tool position unknown', 'err');
  for(const k of ['x', 'y', 'z', 'pitch']) $('cart-' + k).value = lastPose[k].toFixed(1);
  $('cart-yaw').value = '';
  showToast('Filled in the current tool pose');
}

// Relative to the current target pose; the approach stays in the arm plane
async function cartJog(axis, dir){
  const r = await api('/movepose', {[axis]: segCart.value * dir, rel: 1});
  if(r) setResult('MOVING ▸ ' + fmtJoints(r.deg));
  else  setResult('✕ ' + api.lastError, true);
}

// ── Gripper ───────────────────────────────────────────────────────────────────
const sv = $('servo');
let servoDragging = false;
sv.addEventListener('input',  () => { servoDragging = true; $('servo-val').textContent = sv.value; });
sv.addEventListener('change', () => { servoDragging = false; sendServo(+sv.value); });

function setServoUI(v){
  if(v == null) return;
  sv.value = v;
  $('servo-val').textContent = v;
}
async function sendServo(v){
  v = Math.max(700, Math.min(2300, Math.round(v)));
  setServoUI(v);
  if(await api('/servo', {us: v})) showToast(`Gripper → ${v} µs`);
}
const nudgeServo = d => sendServo(+sv.value + d);

// ── Tabs & origin ─────────────────────────────────────────────────────────────
function showTab(t){
  document.querySelectorAll('.tabs [role=tab]').forEach(b => b.setAttribute('aria-selected', b.dataset.tab === t));
  $('tab-tool').hidden   = t !== 'tool';
  $('tab-joints').hidden = t !== 'joints';
  setPref('tab', t);
}
showTab(pref('tab', 'tool'));

confirmTap($('btnOrigin'), async () => {
  if(await api('/home', {})) showToast('Moving all joints to origin');
});

// ── Status rendering (called by pollStatus in app.js) ─────────────────────────
function onStatus(d){
  ST = d;
  lastPose = d.x == null ? null : d;
  for(const k of ['x', 'y', 'z', 'pitch', 'yaw']) $('p-' + k).textContent = lastPose ? fmt(d[k]) : '—';
  $('pose-off').hidden = $('tool-lock').hidden = !!lastPose;
  $('tool-fs').disabled = !lastPose;

  for(let i = 1; i <= N; i++){
    const a = d['a' + i], c = cal(i);
    $('led-' + i).className = 'led' + (d['m' + i] ? ' busy' : '');
    $('ang-' + i).innerHTML = a == null ? '<span class="badge warn">No cal</span>' : fmt(a) + '<span class="unit">°</span>';
    $('stp-' + i).textContent = (d['j' + i] ?? '—') + ' st';
    const mk = $('mk-' + i);
    if(mk && a != null && c.max > c.min){
      const p = pct(c, a);
      mk.hidden = false;
      mk.style.left = Math.max(0, Math.min(100, p)) + '%';
      $('mtr-' + i).classList.toggle('over', p < 0 || p > 100);
    }
  }
  renderJointValues();
  if(!servoDragging) setServoUI(d.servo);
}

// ── Startup ───────────────────────────────────────────────────────────────────
async function loadCal(){
  const r = await api('/calib');
  if(!r) return;
  CAL = r;
  renderMeters();
  renderJointCtl();
  if(ST.j1 != null) onStatus(ST);
}
renderJointCtl();
loadCal();
loadConfig();
</script>
</body>
</html>
)rawhtml";
