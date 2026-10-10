#pragma once

#include <Arduino.h>

/* Browser-tab icon, served at /favicon.svg. Same mark as logo/favicon.svg
   in the repo root (heavier strokes so it stays legible at 16 px).
   Included only by web_server.cpp. */
static const char FAVICON_SVG[] PROGMEM = R"rawsvg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="32" height="32">
  <defs>
    <linearGradient id="g" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#12a3b8"/>
      <stop offset="1" stop-color="#005b6b"/>
    </linearGradient>
  </defs>
  <path d="M14 2H62V50L50 62H2V14Z" fill="url(#g)"/>
  <g fill="none" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round">
    <path d="M17 19L32 37L47 19" stroke-width="9" stroke-opacity=".55"/>
    <path d="M17 46V19M47 46V19" stroke-width="9"/>
  </g>
  <g fill="#007d90">
    <circle cx="17" cy="19" r="2.5"/>
    <circle cx="47" cy="19" r="2.5"/>
  </g>
</svg>
)rawsvg";
