#include "connectivity.h"

#include <ArduinoOTA.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>  // for esp_wifi_set_ps() / WIFI_PS_NONE

#include "movens_config.h"

static const char *apSsid = APSSID;
static const char *apPass = APPSK;

static bool     staMode   = false;
static uint32_t restartAt = 0;  // millis() of a scheduled restart, 0 = none

static bool             joinFailed     = false;  // the saved network couldn't be joined at boot
static volatile uint8_t joinFailReason = 0;      // wifi_err_reason_t of its last attempt, 0 = none seen

/* ── Saved station credentials (NVS namespace "wifi") ── */
static void loadCredentials(String& ssid, String& pass) {
  Preferences p;
  if (p.begin("wifi", true)) {
    ssid = p.getString("ssid", "");
    pass = p.getString("pass", "");
    p.end();
  }
}

String wifiSavedSsid() {
  String ssid, pass;
  loadCredentials(ssid, pass);
  return ssid;
}

bool wifiSaveCredentials(const String& ssid, const String& pass) {
  Preferences p;
  if (!p.begin("wifi", false)) return false;
  bool ok;
  if (ssid.length()) {
    ok = p.putString("ssid", ssid) == ssid.length() &&
         p.putString("pass", pass) == pass.length();
  } else {
    ok = p.clear();
  }
  p.end();
  return ok;
}

bool wifiStationMode() { return staMode; }

String wifiJoinError() {
  if (!joinFailed) return "";
  switch (joinFailReason) {
    case 0:
      return "no reply from the network";
    case WIFI_REASON_NO_AP_FOUND:
    case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
      return "network not found (the robot only sees 2.4 GHz WiFi)";
    case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
    case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
      return "the network's security mode isn't supported";
    case WIFI_REASON_AUTH_FAIL:
    case WIFI_REASON_MIC_FAILURE:
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
      return "the password was rejected";
    default:
      return "WiFi error " + String(joinFailReason);
  }
}

void scheduleRestart(uint32_t delayMs) {
  restartAt = millis() + delayMs;
  if (restartAt == 0) restartAt = 1;
}

// Disable ALL power-saving modes — the #1 cause of ESP32 AP dropouts.
// Modem sleep lets the radio go quiet between beacons; under motor ISR
// load the radio sometimes misses its wake window and the AP vanishes.
// Also keeps station-mode latency low.
static void radioFullPower() {
  WiFi.setSleep(false);                    // Arduino-level modem sleep off
  esp_wifi_set_ps(WIFI_PS_NONE);           // IDF-level power saving off
  WiFi.setTxPower(WIFI_POWER_19_5dBm);    // maximum TX power
}

static bool joinNetwork(const String& ssid, const String& pass) {
  Serial.printf("Joining WiFi \"%s\"", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  // Remember why the driver gave up, for the Network page
  joinFailReason = 0;
  wifi_event_id_t watch = WiFi.onEvent(
    [](arduino_event_id_t, arduino_event_info_t info) {
      joinFailReason = info.wifi_sta_disconnected.reason;
    },
    ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // WiFi.begin() doesn't set the WPA3 key-derivation method. Store the config
  // without connecting, allow both methods so routers that require
  // hash-to-element accept the join, then connect.
  WiFi.begin(ssid.c_str(), pass.c_str(), 0, nullptr, false);
  wifi_config_t conf;
  if (esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK) {
    conf.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    esp_wifi_set_config(WIFI_IF_STA, &conf);
  }
  esp_wifi_connect();
  radioFullPower();

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  WiFi.removeEvent(watch);
  return WiFi.status() == WL_CONNECTED;
}

static void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, apPass);
  radioFullPower();
}

void setupWiFi() {
  WiFi.persistent(false);          // credentials live in our own NVS namespace
  WiFi.setHostname(WIFI_HOSTNAME);

  String ssid, pass;
  loadCredentials(ssid, pass);

  if (ssid.length() && joinNetwork(ssid, pass)) {
    staMode = true;
    Serial.print("WiFi connected, IP address: ");
    Serial.println(WiFi.localIP());
    return;
  }

  if (ssid.length()) {
    joinFailed = true;
    Serial.println("WiFi connection failed: " + wifiJoinError() + " — starting access point");
    WiFi.disconnect(true);
  }
  startAccessPoint();
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void setupOTA() {
  ArduinoOTA.setHostname(WIFI_HOSTNAME);  // also advertised as <name>.local
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
      // using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

  ArduinoOTA.begin();
}

void handleNetwork() {
  ArduinoOTA.handle();

  uint32_t now = millis();
  if (restartAt && (int32_t)(now - restartAt) >= 0) {
    Serial.println("Restarting...");
    ESP.restart();
  }

  // Station mode: the WiFi driver reconnects on its own (setAutoReconnect).
  if (staMode) return;

  // ── AP watchdog ───────────────────────────────
  // Restart the softAP if its IP disappears (rare but possible under heavy
  // motor ISR load). Checked every 5 s to avoid any overhead.
  static uint32_t lastApCheck = 0;
  if (now - lastApCheck >= 5000) {
    lastApCheck = now;
    if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
      Serial.println("[watchdog] AP down — restarting softAP");
      WiFi.softAP(apSsid, apPass);
      WiFi.setSleep(false);
      esp_wifi_set_ps(WIFI_PS_NONE);
    }
  }
}
