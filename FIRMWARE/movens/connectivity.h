#pragma once

#include <Arduino.h>

/* ─────────────────────────────────────────────
   WiFi (station with access-point fallback) + OTA updates
   ───────────────────────────────────────────── */

// Join the saved network; start the access point if there is none or it fails
void setupWiFi();
void setupOTA();

// Call every loop(): services OTA, restarts the AP if it drops and runs a
// pending restart
void handleNetwork();

// true when joined to the saved network, false when running the access point
bool wifiStationMode();

// Network saved in NVS ("" if none)
String wifiSavedSsid();

// Save the network to join from the next boot on; an empty ssid forgets it
// (always start the access point). Returns false if NVS can't be written.
bool wifiSaveCredentials(const String& ssid, const String& pass);

// Restart the ESP32 from loop() after delayMs (lets the HTTP reply go out)
void scheduleRestart(uint32_t delayMs);
