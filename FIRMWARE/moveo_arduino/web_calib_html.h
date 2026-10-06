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
<link rel="icon" type="image/svg+xml" href="/favicon.svg"/>
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
          <h3>Zero at the alignment pose</h3>
          <p class="hint">Put the joint on its alignment marks &mdash; the pose it must be in whenever the robot is
            powered on &mdash; and zero the step counter there. Skip this if the robot was powered on with the
            joint on its marks.</p>
          <div class="actions">
            <button class="btn primary" onclick="calZero()">Zero counter here</button>
          </div>
        </div>
      </li>

      <li class="step" id="st-2">
        <div class="step-n">2</div>
        <div class="step-b">
          <h3>Record two known positions</h3>
          <p class="hint">Jog to a pose whose angle you can measure, enter that angle and record it. Do the same
            at a second pose, as far from the first as the joint allows. Approach both poses from the same
            direction so backlash doesn't skew the result.</p>
          <div class="pts">
            <div class="pt" id="pt-a">
              <span class="pt-tag">A</span>
              <label class="field"><span>Angle</span>
                <span class="inp"><input type="number" id="pa-deg" step="any" oninput="onPointDeg('a')"/><i>&deg;</i></span></label>
              <div class="pt-st"><span>Steps</span><span class="v" id="pa-steps">—</span></div>
              <button class="btn" onclick="calRecord('a')">Record here</button>
            </div>
            <div class="pt" id="pt-b">
              <span class="pt-tag">B</span>
              <label class="field"><span>Angle</span>
                <span class="inp"><input type="number" id="pb-deg" step="any" oninput="onPointDeg('b')"/><i>&deg;</i></span></label>
              <div class="pt-st"><span>Steps</span><span class="v" id="pb-steps">—</span></div>
              <button class="btn" onclick="calRecord('b')">Record here</button>
            </div>
          </div>
          <div class="fit" id="cal-fit"></div>
        </div>
      </li>

      <li class="step" id="st-3">
        <div class="step-n">3</div>
        <div class="step-b">
          <h3>Motion, limits &amp; save</h3>
          <div class="lim-grid">
            <label class="field"><span>Steps per degree (sign = direction)</span>
              <span class="inp"><input type="number" id="cal-spd" value="0" step="any" oninput="updateDirty()"/><i>st/&deg;</i></span></label>
            <label class="field"><span>Angle at alignment pose</span>
              <span class="inp"><input type="number" id="cal-home" value="0" step="any" oninput="updateDirty()"/><i>&deg;</i></span></label>
          </div>
          <div class="lim-grid">
            <label class="field"><span>Max speed</span>
              <span class="inp wide"><input type="number" id="cal-speed" min="1" step="1" oninput="updateDirty()"/><i>st/s</i></span></label>
            <label class="field"><span>Acceleration</span>
              <span class="inp wide"><input type="number" id="cal-accel" min="1" step="1" oninput="updateDirty()"/><i>st/s&sup2;</i></span></label>
          </div>
          <div class="fit note" id="cal-motion"></div>
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
          <p class="hint">Speed and acceleration are the joint's defaults at power-on and take effect as soon as
            you save. Lower them if the motor skips steps. Motion settings on the Control page override them until
            the next restart.</p>
        </div>
      </li>

      <li class="step" id="st-4">
        <div class="step-n">4</div>
        <div class="step-b">
          <h3>Verify</h3>
          <div class="actions">
            <button class="btn" onclick="calGoPoint('a')">Go to A</button>
            <button class="btn" onclick="calGoPoint('b')">Go to B</button>
            <button class="btn ghost" onclick="calGoReference()">Back to alignment pose</button>
          </div>
          <p class="hint">Measure the angle at A and B again with the same tool, then go back and check the
            alignment marks line up. A consistent angle error means redo step 2. Marks that don't line up again
            mean the motor is skipping steps: lower speed/accel or raise the driver current.</p>
        </div>
      </li>
    </ol>
    <div class="card-body">
      <details>
        <summary>How to measure angles on each joint</summary>
        <div class="dbody"><ul>
          <li>The two positions can be any angles, e.g. 0&deg; and 90&deg;, or &minus;45&deg; and +60&deg;. The
            wider apart they are, the more accurate the result.</li>
          <li><b>J2 shoulder:</b> a phone inclinometer app on the upper arm reads its tilt from vertical, which is
            the J2 angle.</li>
          <li><b>J3 elbow, J5 wrist pitch:</b> the angle is relative to the previous link. Keep that link vertical,
            or measure both links and subtract.</li>
          <li><b>J1 base:</b> a square or protractor against the base, or angle marks on the base and the
            turntable.</li>
          <li><b>J4 wrist roll:</b> with the forearm horizontal, an inclinometer on the flat face of the gripper.</li>
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
          <li>The alignment pose doesn't have to be straight up: its angle is worked out from the two recorded
            positions.</li>
        </ul></div>
      </details>
    </div>
  </section>
</main>

<div class="toast" id="toast" role="status" aria-live="polite"></div>

<script src="/app.js"></script>
<script>
// Model: angle = home + steps / spd, with step 0 at the alignment pose (where
// the joint sits at power-on). Two recorded poses of known angle give
// spd = Δsteps / Δangle and home = angleA − stepsA / spd.
// Only spd/home/limits and the speed/accel profile are stored on the robot.
let CAL = {};       // robot calibration, keyed 'j1'..'j5'
let STATUS = {};    // last /status reply
let curJ = pref('calJoint', 1);
const PROG = {};    // per joint: steps finished in this session {zero, meas, saved, verify, pts}

const calJ   = () => curJ;
const num    = id => parseFloat($(id).value);
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
    if(!PROG['j' + j]){ const ok = isCal(CAL['j' + j]); PROG['j' + j] = {zero: ok, meas: ok, saved: ok}; }
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
    $('cal-speed').value = c.speed;
    $('cal-accel').value = c.accel;
  }
  $('cal-jname').textContent = `J${curJ} ${JOINT_NAMES[curJ - 1]}`;
  renderPoints();
  renderFit(fit());
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
const STEP_KEYS = ['zero', 'meas', 'saved', 'verify'];
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
  return !(near(num('cal-spd'), c.spd, 1e-5) && near(num('cal-home'), c.home, 0.005) &&
           near(num('cal-min'), c.min, 0.005) && near(num('cal-max'), c.max, 0.005) &&
           near(num('cal-speed'), c.speed, 0.5) && near(num('cal-accel'), c.accel, 0.5) &&
           $('cal-lim').checked === !!c.limits);
}
function updateDirty(){ $('cal-dirty').hidden = !isDirty(); renderMotion(); }

// Speed / accel in joint degrees, once steps per degree is known
function renderMotion(){
  const spd = Math.abs(num('cal-spd')), v = num('cal-speed'), a = num('cal-accel');
  const ok = spd > 1e-6 && v > 0 && a > 0;
  $('cal-motion').textContent = ok
    ? `≈ ${(v / spd).toFixed(1)} °/s max, ${(a / spd).toFixed(1)} °/s² · 0 → full speed in ${(v / a).toFixed(2)} s`
    : '';
}

// ── Two-point fit ─────────────────────────────────────────────────────────────
// Recorded positions of the current joint: {a: {deg, steps}, b: {deg, steps}}.
// steps is null until recorded; steps are relative to the current zero.
function pts(){
  const p = prog();
  return p.pts || (p.pts = {a: {deg: 0, steps: null}, b: {deg: 90, steps: null}});
}

function renderPoints(){
  const P = pts();
  for(const k of ['a', 'b']){
    $(`p${k}-deg`).value = Number.isFinite(P[k].deg) ? P[k].deg : '';
    $(`p${k}-steps`).textContent = P[k].steps ?? '—';
    $('pt-' + k).classList.toggle('rec', P[k].steps != null);
  }
}

// {spd, home, span} from the two positions, or {msg} / {err} explaining why not
function fit(){
  const {a, b} = pts();
  if(a.steps == null && b.steps == null) return {msg: ''};
  if(a.steps == null || b.steps == null) return {msg: `Now record position ${a.steps == null ? 'A' : 'B'}.`};
  if(!Number.isFinite(a.deg) || !Number.isFinite(b.deg)) return {err: 'Enter the angle of both positions.'};
  const dDeg = b.deg - a.deg, dSteps = b.steps - a.steps;
  if(Math.abs(dDeg) < 1) return {err: 'The two angles must be at least 1° apart.'};
  if(Math.abs(dSteps) < 10) return {err: 'The joint barely moved between A and B — jog further.'};
  const spd = dSteps / dDeg;
  return {spd, home: a.deg - a.steps / spd, span: Math.abs(dDeg)};
}

function renderFit(f){
  const el = $('cal-fit');
  if(f.spd === undefined){
    el.textContent = f.err || f.msg;
    el.className = 'fit' + (f.err ? ' err' : ' note');
    return;
  }
  const narrow = f.span < 30;
  el.textContent = `${Math.round(f.span * 100) / 100}° span → ${f.spd.toFixed(4)} steps/°, alignment pose at ` +
    `${f.home.toFixed(2)}°.` + (narrow ? ' The positions are close together; a wider span is more accurate.' : '') +
    ' Save to store it.';
  el.className = 'fit' + (narrow ? ' warn' : '');
}

// Recompute and copy the result into the form of step 3
function applyFit(){
  const f = fit();
  renderFit(f);
  if(f.spd === undefined){ prog().meas = false; renderSteps(); return; }
  $('cal-spd').value  = f.spd.toFixed(5);
  $('cal-home').value = f.home.toFixed(2);
  markStep('meas', {saved: false, verify: false});
  updateDirty();
}

function onPointDeg(k){
  const P = pts();
  P[k].deg = num(`p${k}-deg`);
  if(P.a.steps != null && P.b.steps != null) applyFit();
}

// ── Actions ───────────────────────────────────────────────────────────────────
async function calJog(dir){
  await api('/move', {joint: calJ(), steps: segCal.value * dir});
}

async function calZero(){
  const j = calJ();
  if(!await api('/setpos', {joint: j, steps: 0})) return;
  delete prog().pts;            // recorded steps were relative to the old zero
  renderPoints();
  renderFit(fit());
  markStep('zero', {verify: false});
  showToast(`J${j}: alignment pose = 0 steps`);
}

async function calRecord(k){
  const j = calJ(), P = pts();
  P[k].deg = num(`p${k}-deg`);
  if(!Number.isFinite(P[k].deg)) return showToast(`Enter the angle of position ${k.toUpperCase()} first`, 'err');
  const d = await api('/status');          // fresh reading, not the last poll
  if(!d) return;
  if(d['m' + j]) return showToast('Wait until the joint stops', 'err');
  P[k].steps = d['j' + j];
  renderPoints();
  applyFit();
  showToast(`Position ${k.toUpperCase()}: ${P[k].deg}° at ${P[k].steps} steps`);
}

async function calSetLimit(which){
  const j = calJ(), spd = num('cal-spd'), home = num('cal-home');
  if(!Number.isFinite(spd) || Math.abs(spd) < 1e-9) return showToast('Record the two positions first', 'err');
  const d = await api('/status');
  if(!d) return;
  $('cal-' + which).value = (home + d['j' + j] / spd).toFixed(1);
  updateDirty();
}

async function calSave(){
  const j = calJ();
  const body = {joint: j, spd: num('cal-spd'), home: num('cal-home'),
                min: num('cal-min'), max: num('cal-max'),
                limits: $('cal-lim').checked ? 1 : 0,
                speed: Math.round(num('cal-speed')), accel: Math.round(num('cal-accel'))};
  if(![body.spd, body.home, body.min, body.max].every(Number.isFinite))
    return showToast('All calibration fields must be numbers', 'err');
  if(!(body.speed >= 1 && body.accel >= 1))
    return showToast('Speed and acceleration must be at least 1', 'err');
  if(await api('/calib', body)){
    showToast(`J${j} calibration saved`);
    markStep('saved', {verify: false});
    await loadCal(false);
  }
}

async function calGoPoint(k){
  const j = calJ(), deg = pts()[k].deg;
  if(!Number.isFinite(deg)) return showToast(`Enter the angle of position ${k.toUpperCase()}`, 'err');
  if(await api('/moveangle', {joint: j, deg})){ showToast(`J${j} → ${deg}°`); markStep('verify'); }
}

async function calGoReference(){
  if(await api('/moveto', {joint: calJ(), pos: 0})) showToast(`J${calJ()} → alignment pose`);
}

$('cal-jname').textContent = `J${curJ} ${JOINT_NAMES[curJ - 1]}`;
renderPoints();
loadCal(true);
</script>
</body>
</html>
)rawhtml";
