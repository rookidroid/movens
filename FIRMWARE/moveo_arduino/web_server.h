#pragma once

/* ─────────────────────────────────────────────
   Async Web Server  (non-blocking, Core 0)
     GET  /, /calibrate, /app.css, /app.js
     GET  /status, /calib
     POST /move, /moveto, /moveangle, /setpos, /config,
          /stop, /home, /servo, /calib
   ───────────────────────────────────────────── */

void setupWebServer();
