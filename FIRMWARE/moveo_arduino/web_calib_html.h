#pragma once

#include <Arduino.h>

/* Joint calibration page, served at /calibrate.
   Included only by web_server.cpp. */
static const char CALIB_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Moveo Calibration</title>
<link rel="stylesheet" href="/app.css"/>
</head>
<body data-page="calibrate">
<header class="appbar" id="appbar"></header>

<main class="layout calib">

  <div class="side">
  <!-- 01 · Joint picker -->
  <section class="card area-pick">
    <div class="card-head"><span class="idx">01</span><h2>Select joint</h2></div>
    <div class="card-body"><div class="jpick" id="jpick" role="tablist"></div></div>
  </section>

  <!-- 02 · Live position & jog -->
  <section class="card area-live">
    <div class="card-head">
      <span class="idx">02</span><h2>Live &middot; <span id="cal-jname">J1 Base</span></h2><span class="spacer"></span>
      <span class="led" id="cal-led" title="Moving"></span>
    </div>
    <div class="card-body">
      <div class="readout2">
        <div><span class="k">Steps</span><span class="v" id="cal-steps">—</span></div>
        <div><span class="k">Angle &deg;</span><span class="v" id="cal-ang">—</span></div>
      </div>
      <div class="jogrow">
        <button class="btn jog" onclick="calJog(-1)">&minus; Jog</button>
        <div id="seg-cal" aria-label="Jog step (steps)"></div>
        <button class="btn jog" onclick="calJog( 1)">+ Jog</button>
      </div>
      <p class="hint">Jog step is in motor steps. Press <b>Esc</b> or <b>STOP</b> at any time.</p>
    </div>
  </section>

  </div>

  <!-- 03 · Procedure -->
  <section class="card area-proc">
    <div class="card-head">
      <span class="idx">03</span><h2>Calibration procedure</h2><span class="spacer"></span>
      <span class="badge" id="cal-state">—</span>
    </div>
    <ol class="steps">
      <li class="step" id="st-1">
        <div class="step-n">1</div>
        <div class="step-b">
          <h3>Set reference</h3>
          <p class="hint">Put the joint on its alignment marks and enter the angle that pose represents (usually 0).
            The current position becomes 0 steps.</p>
          <div class="row">
            <label class="field"><span>Alignment angle</span>
              <span class="inp"><input type="number" id="cal-home" value="0" step="any"/><i>&deg;</i></span></label>
            <button class="btn primary" onclick="calSetReference()">Set reference</button>
          </div>
        </div>
      </li>

      <li class="step" id="st-2">
        <div class="step-n">2</div>
        <div class="step-b">
          <h3>Measure 90&deg;</h3>
          <p class="hint">Jog until the joint is exactly 90&deg; from the alignment pose, then press the direction you
            turned. Finish with small steps in one direction; if you overshoot, back off past 90&deg; and come in
            again so backlash doesn't skew the reading.</p>
          <div class="actions">
            <button class="btn primary" onclick="calAt90( 1)">Joint is at +90&deg;</button>
            <button class="btn" onclick="calAt90(-1)">Joint is at &minus;90&deg;</button>
          </div>
          <div class="fit" id="cal-fit"></div>
        </div>
      </li>

      <li class="step" id="st-3">
        <div class="step-n">3</div>
        <div class="step-b">
          <h3>Limits &amp; save</h3>
          <label class="field"><span>Steps per degree (sign = direction)</span>
            <span class="inp"><input type="number" id="cal-spd" value="0" step="any" oninput="updateDirty()"/><i>st/&deg;</i></span></label>
          <div class="lim-grid">
            <div class="field"><span>Min</span>
              <span class="inp"><input type="number" id="cal-min" value="-180" step="any" oninput="updateDirty()"/><i>&deg;</i></span>
              <button type="button" class="btn sm ghost" onclick="calSetLimit('min')">&larr; Use current</button></div>
            <div class="field"><span>Max</span>
              <span class="inp"><input type="number" id="cal-max" value="180" step="any" oninput="updateDirty()"/><i>&deg;</i></span>
              <button type="button" class="btn sm ghost" onclick="calSetLimit('max')">&larr; Use current</button></div>
          </div>
          <div class="actions">
            <label class="check"><input type="checkbox" id="cal-lim" onchange="updateDirty()"/> Enforce soft limits</label>
            <span class="grow"></span>
            <span class="badge warn" id="cal-dirty" hidden>Unsaved</span>
            <button class="btn primary" onclick="calSave()">&#10003; Save to robot</button>
          </div>
        </div>
      </li>

      <li class="step" id="st-4">
        <div class="step-n">4</div>
        <div class="step-b">
          <h3>Verify</h3>
          <div class="actions">
            <button class="btn" onclick="calGo90( 1)">Turn to +90&deg;</button>
            <button class="btn" onclick="calGo90(-1)">Turn to &minus;90&deg;</button>
            <button class="btn ghost" onclick="calGoReference()">Back to reference</button>
          </div>
          <p class="hint">Check 90&deg; with the same tool, then go back and check the alignment marks line up.
            A consistent error at 90&deg; means redo step 2. Marks that don't line up again mean the motor is
            skipping steps: lower speed/accel or raise the driver current.</p>
        </div>
      </li>
    </ol>
    <div class="card-body">
      <details>
        <summary>How to measure 90&deg; on each joint</summary>
        <div class="dbody"><ul>
          <li><b>J2, J3, J5 (pitch joints):</b> phone inclinometer app or a square on the link, e.g. vertical &rarr;
            horizontal.</li>
          <li><b>J1 base:</b> a square against the base, or 90&deg; marks on the base and the turntable.</li>
          <li><b>J4 wrist roll:</b> with the forearm horizontal, an inclinometer on the flat face of the gripper
            (level &rarr; vertical).</li>
          <li>Turn in whichever direction is free; the button you press sets which way is positive.</li>
          <li>Calibration is saved on the ESP32 and survives power cycles. The step counter does not: power on with
            the arm on its alignment marks.</li>
        </ul></div>
      </details>
      <details>
        <summary>Angle conventions for tool (XYZ) control</summary>
        <div class="dbody"><ul>
          <li>The angles must match the arm model. Arm pointing straight up = all joints 0&deg;.</li>
          <li><b>J1</b> positive counter-clockwise seen from above.</li>
          <li><b>J2, J3, J5</b> positive bending forward (toward the reach).</li>
          <li><b>J4</b> right-handed about the forearm (thumb toward the gripper).</li>
          <li>If the alignment pose isn't straight up, enter its real angle in step 1.</li>
        </ul></div>
      </details>
    </div>
  </section>
</main>

<div class="toast" id="toast" role="status" aria-live="polite"></div>

<script src="/app.js"></script>
<script>
// Model: steps = (deg - home) * spd, with step 0 at the alignment pose.
// Each joint is calibrated by turning it exactly 90° from the alignment pose:
// spd = steps / (±90). Only spd/home/limits are stored on the robot.
let CAL = {};       // robot calibration, keyed 'j1'..'j5'
let STATUS = {};    // last /status reply
let curJ = pref('calJoint', 1);
const PROG = {};    // per joint: steps finished in this session {ref, meas, saved, verify}

const calJ   = () => curJ;
const num    = id => parseFloat($(id).value);
const setFit = t => $('cal-fit').textContent = t;
const segCal = seg($('seg-cal'), [1, 10, 100, 1000], pref('calStep', 100), v => setPref('calStep', v));

$('jpick').innerHTML = JOINT_NAMES.map((n, k) => { const i = k + 1; return `
  <button class="jp" role="tab" id="jp-${i}" onclick="pickJoint(${i})">
    <span class="jtag">J${i}</span><span class="nm">${n}</span><span class="st" id="jps-${i}">—</span>
  </button>`; }).join('');

async function loadCal(fill){
  const r = await api('/calib');
  if(!r) return;
  CAL = r;
  for(let j = 1; j <= JOINT_NAMES.length; j++){
    // A joint calibrated earlier starts with steps 1–3 done
    if(!PROG['j' + j]){ const ok = isCal(CAL['j' + j]); PROG['j' + j] = {ref: ok, meas: ok, saved: ok}; }
  }
  renderJointOptions();
  if(fill) fillCalForm(); else { updateDirty(); renderCalPos(); }
}

// Show each joint's calibration state in the picker
function renderJointOptions(){
  for(let i = 1; i <= JOINT_NAMES.length; i++){
    const c = CAL['j' + i], ok = isCal(c), el = $('jps-' + i);
    el.innerHTML = ok ? `✓<span class="spd"> ${c.spd.toFixed(2)}/°</span>` : 'Not set';
    el.className = 'st' + (ok ? ' ok' : '');
    $('jp-' + i).setAttribute('aria-selected', i === curJ);
  }
}

function pickJoint(i){
  if(i === curJ) return;
  if(isDirty() && !confirm(`Discard unsaved calibration changes for J${curJ}?`)) return;
  curJ = i;
  setPref('calJoint', i);
  renderJointOptions();
  fillCalForm();
}

function fillCalForm(){
  const c = CAL['j' + calJ()];
  if(c){
    $('cal-home').value  = c.home;
    $('cal-spd').value   = c.spd;
    $('cal-min').value   = c.min;
    $('cal-max').value   = c.max;
    $('cal-lim').checked = c.limits;
  }
  $('cal-jname').textContent = `J${curJ} ${JOINT_NAMES[curJ - 1]}`;
  setFit('');
  renderCalPos();
  renderSteps();
  updateDirty();
}

function renderCalPos(){
  const j = calJ(), s = STATUS['j' + j], a = STATUS['a' + j], ok = isCal(CAL['j' + j]);
  $('cal-steps').textContent = s ?? '—';
  $('cal-ang').textContent   = a == null ? '—' : fmt(a);
  $('cal-led').className     = 'led' + (STATUS['m' + j] ? ' busy' : '');
  $('cal-state').textContent = ok ? 'Calibrated' : 'Not calibrated';
  $('cal-state').className   = 'badge ' + (ok ? 'ok' : 'warn');
}

// Status rendering (called by pollStatus in app.js)
function onStatus(d){ STATUS = d; renderCalPos(); }

// ── Wizard progress ───────────────────────────────────────────────────────────
const STEP_KEYS = ['ref', 'meas', 'saved', 'verify'];
const prog = () => PROG['j' + curJ] || (PROG['j' + curJ] = {});

function renderSteps(){
  const p = prog(), next = STEP_KEYS.findIndex(k => !p[k]);
  STEP_KEYS.forEach((k, i) => {
    $('st-' + (i + 1)).classList.toggle('done', !!p[k]);
    $('st-' + (i + 1)).classList.toggle('next', i === next);
  });
}
function markStep(k, extra){ Object.assign(prog(), {[k]: true}, extra); renderSteps(); }

// Form differs from the calibration stored on the robot
function isDirty(){
  const c = CAL['j' + calJ()];
  if(!c) return false;
  const near = (a, b, tol) => Math.abs(a - b) <= tol;
  return !(near(num('cal-spd'), c.spd, 1e-5) && near(num('cal-min'), c.min, 0.005) &&
           near(num('cal-max'), c.max, 0.005) && $('cal-lim').checked === !!c.limits);
}
function updateDirty(){ $('cal-dirty').hidden = !isDirty(); }

// ── Actions ───────────────────────────────────────────────────────────────────
async function calJog(dir){
  await api('/move', {joint: calJ(), steps: segCal.value * dir});
}

async function calSetReference(){
  const j = calJ(), home = num('cal-home');
  if(!Number.isFinite(home)) return showToast('Enter the alignment angle', 'err');
  if(!await api('/calib',  {joint: j, home})) return;
  if(!await api('/setpos', {joint: j, deg: home})) return;
  if(CAL['j' + j]) CAL['j' + j].home = home;
  setFit('');
  markStep('ref', {verify: false});
  showToast(`J${j}: alignment pose = 0 steps = ${home}°`);
}

// The joint now sits exactly sign*90° from the alignment pose (step 0)
async function calAt90(sign){
  const j = calJ();
  const d = await api('/status');          // fresh reading, not the last poll
  if(!d) return;
  if(d['m' + j]) return showToast('Wait until the joint stops', 'err');
  const steps = d['j' + j];
  if(!steps) return showToast('Joint is still at 0 steps — jog it to 90° first', 'err');
  const spd = steps / (sign * 90);
  $('cal-spd').value = spd.toFixed(5);
  setFit(`${steps} steps for ${sign > 0 ? '+' : '−'}90° → ${spd.toFixed(4)} steps/°. Save to store it.`);
  markStep('meas', {saved: false, verify: false});
  updateDirty();
}

async function calSetLimit(which){
  const j = calJ(), spd = num('cal-spd'), home = num('cal-home');
  if(!Number.isFinite(spd) || Math.abs(spd) < 1e-9) return showToast('Do the 90° step first', 'err');
  const d = await api('/status');
  if(!d) return;
  $('cal-' + which).value = (home + d['j' + j] / spd).toFixed(1);
  updateDirty();
}

async function calSave(){
  const j = calJ();
  const body = {joint: j, spd: num('cal-spd'), home: num('cal-home'),
                min: num('cal-min'), max: num('cal-max'),
                limits: $('cal-lim').checked ? 1 : 0};
  if(![body.spd, body.home, body.min, body.max].every(Number.isFinite))
    return showToast('All calibration fields must be numbers', 'err');
  if(await api('/calib', body)){
    showToast(`J${j} calibration saved`);
    markStep('saved', {verify: false});
    await loadCal(false);
  }
}

async function calGo90(sign){
  const j = calJ(), deg = num('cal-home') + sign * 90;
  if(await api('/moveangle', {joint: j, deg})){ showToast(`J${j} → ${deg}°`); markStep('verify'); }
}

async function calGoReference(){
  if(await api('/moveto', {joint: calJ(), pos: 0})) showToast(`J${calJ()} → alignment pose`);
}

$('cal-jname').textContent = `J${curJ} ${JOINT_NAMES[curJ - 1]}`;
loadCal(true);
</script>
</body>
</html>
)rawhtml";
