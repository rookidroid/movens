#pragma once

#include <Arduino.h>

/* Stylesheet shared by all pages, served at /app.css.
   Included only by web_server.cpp. */
static const char APP_CSS[] PROGMEM = R"rawcss(
  :root{
    --bg:#0d1117;--surface:#161b22;--surface2:#21262d;
    --accent:#58a6ff;--accent2:#3fb950;--danger:#f85149;
    --warn:#e3b341;--text:#e6edf3;--muted:#8b949e;
    --radius:12px;--gap:16px;
  }
  *{box-sizing:border-box;margin:0;padding:0;}
  body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh;padding:20px;}
  h1{text-align:center;font-size:1.8rem;font-weight:700;letter-spacing:.05em;
     background:linear-gradient(90deg,var(--accent),var(--accent2));
     -webkit-background-clip:text;-webkit-text-fill-color:transparent;margin-bottom:6px;}
  .subtitle{text-align:center;color:var(--muted);font-size:.85rem;margin-bottom:16px;}
  .nav{display:flex;gap:8px;justify-content:center;margin-bottom:20px;}
  .nav a{color:var(--muted);text-decoration:none;font-size:.85rem;font-weight:600;padding:6px 16px;
         border-radius:20px;border:1px solid var(--surface2);}
  .nav a:hover{color:var(--text);border-color:var(--muted);}
  .nav a.active{color:var(--accent);border-color:var(--accent);}
  .top-bar{display:flex;gap:12px;justify-content:center;margin-bottom:28px;flex-wrap:wrap;}
  button{cursor:pointer;border:none;border-radius:8px;font-size:.85rem;font-weight:600;
         padding:9px 18px;transition:transform .1s,opacity .15s;}
  button:active{transform:scale(.95);}
  button:disabled{opacity:.4;cursor:not-allowed;}
  .btn-danger{background:var(--danger);color:#fff;}
  .btn-home{background:var(--surface2);color:var(--text);border:1px solid var(--muted);}
  .btn-primary{background:var(--accent);color:#0d1117;}
  .btn-secondary{background:var(--surface2);color:var(--text);border:1px solid var(--muted);}
  .btn-apply{background:var(--warn);color:#0d1117;font-size:.78rem;padding:7px 14px;}
  .btn-home:hover,.btn-secondary:hover{background:var(--surface);}
  .btn-primary:hover{opacity:.85;}
  .btn-danger:hover{opacity:.85;}

  .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(320px,1fr));gap:var(--gap);}
  .card{background:var(--surface);border:1px solid var(--surface2);border-radius:var(--radius);padding:18px;
        display:flex;flex-direction:column;gap:12px;}
  .card-title{font-size:1rem;font-weight:700;display:flex;align-items:center;gap:8px;}
  .badge{font-size:.7rem;padding:3px 8px;border-radius:20px;background:var(--surface2);color:var(--muted);}
  .badge.stepper{border:1px solid var(--accent);color:var(--accent);}
  .badge.servo{border:1px solid var(--accent2);color:var(--accent2);}

  .pos-display{background:var(--bg);border-radius:8px;padding:10px 14px;font-size:1.4rem;
               font-weight:700;letter-spacing:.04em;text-align:center;color:var(--accent);
               border:1px solid var(--surface2);}

  label{font-size:.75rem;color:var(--muted);display:block;margin-bottom:4px;}
  input[type=number],input[type=range]{
    width:100%;background:var(--bg);border:1px solid var(--surface2);border-radius:6px;
    color:var(--text);padding:7px 10px;font-size:.85rem;outline:none;
    transition:border-color .2s;}
  input[type=number]:focus{border-color:var(--accent);}
  input[type=range]{padding:4px 0;accent-color:var(--accent2);}

  .row{display:flex;gap:8px;align-items:flex-end;}
  .row > div{flex:1;}
  .row > button{flex-shrink:0;}

  .config-row{display:grid;grid-template-columns:1fr 1fr auto;gap:8px;align-items:flex-end;}

  .divider{border:none;border-top:1px solid var(--surface2);}
  .servo-val{text-align:center;font-size:.9rem;color:var(--accent2);font-weight:600;margin-top:2px;}

  .conn-dot{display:inline-block;width:8px;height:8px;border-radius:50%;
            background:var(--muted);margin-right:6px;transition:background .4s;}
  .conn-dot.ok{background:var(--accent2);}
  .conn-dot.err{background:var(--danger);}

  .toast{position:fixed;bottom:24px;right:24px;background:var(--surface2);border:1px solid var(--surface);
         border-radius:8px;padding:12px 18px;font-size:.85rem;opacity:0;transform:translateY(10px);
         transition:opacity .3s,transform .3s;pointer-events:none;z-index:999;max-width:280px;}
  .toast.show{opacity:1;transform:translateY(0);}
  .toast.ok{border-left:3px solid var(--accent2);}
  .toast.err{border-left:3px solid var(--danger);}

  .chips{display:flex;gap:6px;margin-top:6px;}
  .chip{background:var(--surface2);color:var(--muted);border:1px solid var(--surface2);
        font-size:.72rem;padding:4px 10px;border-radius:20px;}
  .chip:hover{color:var(--text);border-color:var(--muted);}

  .cal-card{max-width:1200px;margin:0 auto;}
  .cal-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:var(--gap);}
  .cal-step{background:var(--bg);border:1px solid var(--surface2);border-radius:8px;padding:14px;
            display:flex;flex-direction:column;gap:10px;}
  .cal-step h3{font-size:.85rem;font-weight:700;color:var(--accent);}
  .hint{font-size:.75rem;color:var(--muted);line-height:1.45;}
  .hint ul{padding-left:18px;margin-top:6px;}
  select{background:var(--bg);border:1px solid var(--surface2);border-radius:6px;color:var(--text);
         padding:7px 10px;font-size:.85rem;}
  .fit{font-size:.8rem;color:var(--accent2);font-weight:600;}
  .check{display:flex;align-items:center;gap:8px;font-size:.8rem;color:var(--text);}
  details summary{cursor:pointer;font-size:.8rem;color:var(--muted);}

  @media(max-width:480px){.config-row{grid-template-columns:1fr 1fr;} .config-row button{grid-column:1/-1;}}
)rawcss";
