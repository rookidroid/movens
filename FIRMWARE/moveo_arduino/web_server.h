#pragma once

/* ─────────────────────────────────────────────
   Async Web Server  (non-blocking, Core 0)
     GET  /, /calibrate, /network, /app.css, /app.js
     GET  /status, /calib, /config, /wifi, /wifiscan
     POST /move, /moveto, /moveangle, /movejoints, /movepose,
          /setpos, /config, /stop, /home, /servo, /calib, /wifi
   ───────────────────────────────────────────── */

void setupWebServer();
