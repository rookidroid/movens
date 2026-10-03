#pragma once

/* ─────────────────────────────────────────────
   WiFi access point + OTA updates
   ───────────────────────────────────────────── */

void setupWiFi();
void setupOTA();

// Call every loop(): services OTA and restarts the AP if it drops
void handleNetwork();
