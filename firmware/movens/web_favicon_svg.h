#pragma once

#include <Arduino.h>

/* Browser-tab icon, served at /favicon.svg. Same mark as logo/favicon.svg
   in the repo root (heavier strokes so it stays legible at 16 px).
   Included only by web_server.cpp. */
static const char FAVICON_SVG[] PROGMEM = R"rawsvg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="32" height="32">
  <rect x="2" y="2" width="60" height="60" rx="12" fill="#00788a"/>
  <g fill="none" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round">
    <path d="M12 52H36M17 52L19 46H29L31 52" stroke-width="4.5"/>
    <path d="M24 46V38L34 18L48 24V30" stroke-width="6.5"/>
    <path d="M41 31H55M41 31V38L44 41M55 31V38L52 41" stroke-width="4.5"/>
  </g>
  <g fill="#00788a" stroke="#ffffff" stroke-width="2.5">
    <circle cx="24" cy="38" r="3.5"/>
    <circle cx="34" cy="18" r="3.5"/>
    <circle cx="48" cy="24" r="3"/>
  </g>
</svg>
)rawsvg";
