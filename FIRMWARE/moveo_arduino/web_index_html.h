#pragma once

#include <Arduino.h>

/* Joint control page, served at /.
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
<body>

<h1>&#129470; Moveo Control</h1>
<p class="subtitle">
  <span class="conn-dot" id="dot"></span>
  <span id="conn-status">Connecting…</span> &bull; 192.168.4.1
</p>
<nav class="nav">
  <a href="/" class="active">Control</a>
  <a href="/calibrate">Calibration</a>
</nav>

<div class="top-bar">
  <button class="btn-danger" id="btnStop" onclick="stopAll()">&#9632; EMERGENCY STOP</button>
  <button class="btn-home"   id="btnHome" onclick="homeAll()">&#8962; Home All</button>
</div>

<div class="grid" id="grid"></div>
<div class="toast" id="toast"></div>

<script src="/app.js"></script>
<script>
const JOINTS = [
  {id:'j1', label:'Joint 1', type:'stepper'},
  {id:'j2', label:'Joint 2', type:'stepper'},
  {id:'j3', label:'Joint 3', type:'stepper'},
  {id:'j4', label:'Joint 4', type:'stepper'},
  {id:'j5', label:'Joint 5', type:'stepper'},
  {id:'j6', label:'Hand Servo (J6)', type:'servo'},
];

// ── Build cards ───────────────────────────────────────────────────────────────
const grid = document.getElementById('grid');
JOINTS.forEach(j => {
  if(j.type === 'stepper'){
    grid.innerHTML += `
    <div class="card" id="card-${j.id}">
      <div class="card-title">${j.label} <span class="badge stepper">Stepper</span></div>
      <div class="pos-display" id="pos-${j.id}">— steps</div>
      <hr class="divider"/>
      <div>
        <label>Move by steps</label>
        <div class="row">
          <div><input type="number" id="steps-${j.id}" value="100" min="1"/></div>
          <button class="btn-primary"   onclick="move('${j.id}', 1)">+ Move</button>
          <button class="btn-secondary" onclick="move('${j.id}',-1)">&minus; Move</button>
        </div>
        <div class="chips">${stepChips(j.id)}</div>
      </div>
      <div>
        <label>Move to absolute position (steps)</label>
        <div class="row">
          <div><input type="number" id="abs-${j.id}" value="0"/></div>
          <button class="btn-primary" onclick="moveTo('${j.id}')">&#8594; Go</button>
        </div>
      </div>
      <div>
        <label>Move to angle (&deg;) &mdash; needs calibration</label>
        <div class="row">
          <div><input type="number" id="ang-${j.id}" value="0" step="any"/></div>
          <button class="btn-primary" onclick="moveAngle('${j.id}')">&#8594; Go</button>
        </div>
      </div>
      <hr class="divider"/>
      <div>
        <label>Configuration</label>
        <div class="config-row">
          <div>
            <label>Speed (Hz)</label>
            <input type="number" id="speed-${j.id}" value="3000" min="1"/>
          </div>
          <div>
            <label>Accel (steps/s&sup2;)</label>
            <input type="number" id="accel-${j.id}" value="800" min="1"/>
          </div>
          <button class="btn-apply" onclick="applyConfig('${j.id}')">&#10003; Apply</button>
        </div>
      </div>
    </div>`;
  } else {
    grid.innerHTML += `
    <div class="card" id="card-${j.id}">
      <div class="card-title">${j.label} <span class="badge servo">Servo</span></div>
      <div class="pos-display" id="pos-${j.id}">— &micro;s</div>
      <hr class="divider"/>
      <div>
        <label>Pulse Width (700 &ndash; 2300 &micro;s)</label>
        <input type="range" id="slider-j6" min="700" max="2300" value="1500"
               oninput="updateServoLabel(this.value)" onchange="sendServo(this.value)"/>
        <div class="servo-val" id="servo-label">1500 &micro;s</div>
      </div>
    </div>`;
  }
});

// ── Actions ───────────────────────────────────────────────────────────────────
const jIndex = id => parseInt(id.replace('j',''));

async function move(id, dir){
  const steps = parseInt(document.getElementById('steps-'+id).value) * dir;
  const r = await api('/move', {joint: jIndex(id), steps});
  if(r){ setConnected(true); showToast(`${id.toUpperCase()} moved ${steps>0?'+':''}${steps} steps`); }
}

async function moveTo(id){
  const pos = parseInt(document.getElementById('abs-'+id).value);
  const r = await api('/moveto', {joint: jIndex(id), pos});
  if(r){ setConnected(true); showToast(`${id.toUpperCase()} → ${pos}`); }
}

async function moveAngle(id){
  const deg = parseFloat(document.getElementById('ang-'+id).value);
  const r = await api('/moveangle', {joint: jIndex(id), deg});
  if(r){ showToast(`${id.toUpperCase()} → ${deg}°`); }
}

async function applyConfig(id){
  const speed = parseInt(document.getElementById('speed-'+id).value);
  const accel = parseInt(document.getElementById('accel-'+id).value);
  const r = await api('/config', {joint: jIndex(id), speed, accel});
  if(r){ setConnected(true); showToast(`${id.toUpperCase()} config applied`); }
}

async function homeAll(){
  const r = await api('/home', {});
  if(r){ setConnected(true); showToast('Homing all joints…'); }
}

function updateServoLabel(v){
  document.getElementById('servo-label').textContent = v + ' µs';
}

async function sendServo(v){
  const r = await api('/servo', {us: parseInt(v)});
  if(r){ setConnected(true); showToast('Servo → ' + v + ' µs'); }
}

// ── Status rendering (called by pollStatus in app.js) ─────────────────────────
function onStatus(d){
  for(let i=1;i<=5;i++){
    const el = document.getElementById('pos-j'+i);
    const a  = d['a'+i];
    if(el) el.textContent = (d['j'+i] ?? '?') + ' steps' + (a == null ? '' : ' · ' + a.toFixed(1) + '°');
  }
  const sv = document.getElementById('pos-j6');
  if(sv) sv.textContent = (d.servo ?? '?') + ' µs';
}
</script>
</body>
</html>
)rawhtml";
