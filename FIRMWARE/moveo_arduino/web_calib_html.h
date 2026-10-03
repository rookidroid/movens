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
<body>

<h1>&#129470; Moveo Calibration</h1>
<p class="subtitle">
  <span class="conn-dot" id="dot"></span>
  <span id="conn-status">Connecting…</span> &bull; 192.168.4.1
</p>
<nav class="nav">
  <a href="/">Control</a>
  <a href="/calibrate" class="active">Calibration</a>
</nav>

<div class="top-bar">
  <button class="btn-danger" id="btnStop" onclick="stopAll()">&#9632; EMERGENCY STOP</button>
</div>

<div class="card cal-card">
  <div class="card-title">Calibration <span class="badge stepper">steps &harr; degrees</span></div>
  <div class="row">
    <div>
      <label>Joint</label>
      <select id="cal-joint" onchange="fillCalForm()">
        <option value="1">Joint 1</option><option value="2">Joint 2</option>
        <option value="3">Joint 3</option><option value="4">Joint 4</option>
        <option value="5">Joint 5</option>
      </select>
    </div>
    <div class="pos-display" id="cal-pos">— steps</div>
  </div>
  <div>
    <label>Jog</label>
    <div class="row">
      <div><input type="number" id="steps-cal" value="100" min="1"/></div>
      <button class="btn-primary"   onclick="calJog( 1)">+ Move</button>
      <button class="btn-secondary" onclick="calJog(-1)">&minus; Move</button>
    </div>
    <div class="chips" id="chips-cal"></div>
  </div>

  <div class="cal-grid">
    <div class="cal-step">
      <h3>1 &middot; Reference</h3>
      <p class="hint">Put the joint on its alignment marks, enter the angle that pose represents (usually 0),
        then press Set. The current position becomes 0 steps.</p>
      <div class="row">
        <div><label>Alignment angle (&deg;)</label><input type="number" id="cal-home" value="0" step="any"/></div>
        <button class="btn-apply" onclick="calSetReference()">Set reference</button>
      </div>
    </div>

    <div class="cal-step">
      <h3>2 &middot; Turn 90&deg;</h3>
      <p class="hint">Jog the joint until it is exactly 90&deg; from the alignment pose, then press the button for
        the direction you turned. Finish with small steps all in the same direction; if you overshoot, back off past
        90&deg; and come in again so gear backlash doesn't skew the reading.</p>
      <div class="row">
        <button class="btn-primary"   onclick="calAt90( 1)">Joint is at +90&deg;</button>
        <button class="btn-secondary" onclick="calAt90(-1)">Joint is at &minus;90&deg;</button>
      </div>
      <div class="fit" id="cal-fit"></div>
    </div>

    <div class="cal-step">
      <h3>3 &middot; Result &amp; limits</h3>
      <div>
        <label>Steps per degree (sign = direction)</label>
        <input type="number" id="cal-spd" value="0" step="any"/>
      </div>
      <div class="config-row">
        <div><label>Min (&deg;)</label><input type="number" id="cal-min" value="-180" step="any"/></div>
        <div><label>Max (&deg;)</label><input type="number" id="cal-max" value="180" step="any"/></div>
        <div></div>
      </div>
      <div class="chips">
        <button class="chip" onclick="calSetLimit('min')">Current &rarr; min</button>
        <button class="chip" onclick="calSetLimit('max')">Current &rarr; max</button>
      </div>
      <label class="check"><input type="checkbox" id="cal-lim"/> Enforce soft limits</label>
      <button class="btn-apply" onclick="calSave()">&#10003; Save to robot</button>
    </div>

    <div class="cal-step">
      <h3>4 &middot; Verify</h3>
      <div class="row">
        <button class="btn-primary"   onclick="calGo90( 1)">Turn to +90&deg;</button>
        <button class="btn-primary"   onclick="calGo90(-1)">Turn to &minus;90&deg;</button>
      </div>
      <button class="btn-secondary" onclick="calGoReference()">Back to alignment pose (0 steps)</button>
      <p class="hint">After saving, turn to 90&deg; and check it with the same tool, then go back and check that the
        alignment marks line up. A consistent error at 90&deg; means redo step 2. Marks that don't line up again
        mean the motor is skipping steps (lower speed/accel or raise driver current).</p>
    </div>
  </div>

  <details>
    <summary>How to check 90&deg; on each joint</summary>
    <div class="hint"><ul>
      <li><b>J2, J3, J5 (pitch joints):</b> phone inclinometer app or a square on the link. E.g. vertical &rarr;
        horizontal.</li>
      <li><b>J1 base:</b> a square against the base, or 90&deg; marks on the base and the turntable.</li>
      <li><b>J4 wrist roll:</b> with the forearm horizontal, an inclinometer on the flat face of the gripper
        (level &rarr; vertical).</li>
      <li>Turn in whichever direction is free; the button you press sets which way is positive.</li>
      <li><b>For Cartesian (IK) control</b> the angles must match the arm model. Arm straight up = all 0&deg;.
        Positive: J1 counter-clockwise seen from above; J2, J3 and J5 bend forward (toward the reach);
        J4 right-handed about the forearm (thumb toward the gripper). If the alignment pose isn't straight up,
        enter its real angle in step 1.</li>
      <li>Calibration is saved on the ESP32 and survives power cycles. The step counter does not: power on with the
        arm on its alignment marks, as before.</li>
    </ul></div>
  </details>
</div>

<div class="toast" id="toast"></div>

<script src="/app.js"></script>
<script>
// Model: steps = (deg - home) * spd, with step 0 at the alignment pose.
// Each joint is calibrated by turning it exactly 90° from the alignment pose:
// spd = steps / (±90). Only spd/home/limits are stored on the robot.
let CAL = {};       // robot calibration, keyed 'j1'..'j5'
let STATUS = {};    // last /status reply

const calJ  = () => parseInt(document.getElementById('cal-joint').value);
const num   = id => parseFloat(document.getElementById(id).value);
const setFit = t => document.getElementById('cal-fit').textContent = t;
document.getElementById('chips-cal').innerHTML = stepChips('cal');

async function loadCal(fill){
  const r = await api('/calib');
  if(!r) return;
  CAL = r;
  renderJointOptions();
  if(fill) fillCalForm();
}

// Show each joint's calibration state in the selector
function renderJointOptions(){
  document.querySelectorAll('#cal-joint option').forEach(o => {
    const c = CAL['j'+o.value];
    const ok = c && Math.abs(c.spd) > 1e-6;
    o.textContent = `Joint ${o.value} — ` + (ok ? `${c.spd.toFixed(2)} steps/°` : 'not calibrated');
  });
}

function fillCalForm(){
  const c = CAL['j'+calJ()];
  if(c){
    document.getElementById('cal-home').value  = c.home;
    document.getElementById('cal-spd').value   = c.spd;
    document.getElementById('cal-min').value   = c.min;
    document.getElementById('cal-max').value   = c.max;
    document.getElementById('cal-lim').checked = c.limits;
  }
  setFit('');
  renderCalPos();
}

function renderCalPos(){
  const j = calJ(), s = STATUS['j'+j], a = STATUS['a'+j];
  document.getElementById('cal-pos').textContent =
    (s ?? '?') + ' steps' + (a == null ? '' : ' · ' + a.toFixed(1) + '°');
}

// Status rendering (called by pollStatus in app.js)
function onStatus(d){ STATUS = d; renderCalPos(); }

async function calJog(dir){
  const steps = parseInt(document.getElementById('steps-cal').value) * dir;
  if(await api('/move', {joint: calJ(), steps})) setTimeout(pollStatus, 300);
}

async function calSetReference(){
  const j = calJ(), home = num('cal-home');
  if(!Number.isFinite(home)) return showToast('Enter the alignment angle', 'err');
  if(!await api('/calib',  {joint: j, home})) return;
  if(!await api('/setpos', {joint: j, deg: home})) return;
  if(CAL['j'+j]) CAL['j'+j].home = home;
  setFit('');
  showToast(`J${j}: alignment pose = 0 steps = ${home}°`);
  setTimeout(pollStatus, 200);
}

// The joint now sits exactly sign*90° from the alignment pose (step 0)
async function calAt90(sign){
  const j = calJ();
  const d = await api('/status');          // fresh reading, not the last poll
  if(!d) return;
  if(d['m'+j]) return showToast('Wait until the joint stops', 'err');
  const steps = d['j'+j];
  if(!steps) return showToast('Joint is still at 0 steps — jog it to 90° first', 'err');
  const spd = steps / (sign * 90);
  document.getElementById('cal-spd').value = spd.toFixed(5);
  setFit(`${steps} steps for ${sign > 0 ? '+' : '−'}90° → ${spd.toFixed(4)} steps/°. Press Save to store it.`);
}

async function calSetLimit(which){
  const j = calJ(), spd = num('cal-spd'), home = num('cal-home');
  if(!Number.isFinite(spd) || Math.abs(spd) < 1e-9) return showToast('Do the 90° step first', 'err');
  const d = await api('/status');
  if(!d) return;
  document.getElementById('cal-'+which).value = (home + d['j'+j]/spd).toFixed(1);
}

async function calSave(){
  const j = calJ();
  const body = {joint: j, spd: num('cal-spd'), home: num('cal-home'),
                min: num('cal-min'), max: num('cal-max'),
                limits: document.getElementById('cal-lim').checked ? 1 : 0};
  if(![body.spd, body.home, body.min, body.max].every(Number.isFinite))
    return showToast('All calibration fields must be numbers', 'err');
  if(await api('/calib', body)){
    showToast(`J${j} calibration saved`);
    await loadCal(false);
    pollStatus();
  }
}

async function calGo90(sign){
  const j = calJ(), deg = num('cal-home') + sign * 90;
  if(await api('/moveangle', {joint: j, deg})) showToast(`J${j} → ${deg}°`);
}

async function calGoReference(){
  if(await api('/moveto', {joint: calJ(), pos: 0})) showToast(`J${calJ()} → alignment pose`);
}

loadCal(true);
</script>
</body>
</html>
)rawhtml";
