#include "connectivity.h"

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <esp_wifi.h>  // for esp_wifi_set_ps() / WIFI_PS_NONE

#include "moveo_config.h"

static const char *ssid     = APSSID;
static const char *password = APPSK;

void setupWiFi() {
  WiFi.persistent(false);          // avoid unnecessary flash writes
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  // Disable ALL power-saving modes — the #1 cause of ESP32 AP dropouts.
  // Modem sleep lets the radio go quiet between beacons; under motor ISR
  // load the radio sometimes misses its wake window and the AP vanishes.
  WiFi.setSleep(false);                    // Arduino-level modem sleep off
  esp_wifi_set_ps(WIFI_PS_NONE);           // IDF-level power saving off
  WiFi.setTxPower(WIFI_POWER_19_5dBm);    // maximum TX power

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void setupOTA() {
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

  // ── AP watchdog ───────────────────────────────
  // Restart the softAP if its IP disappears (rare but possible under heavy
  // motor ISR load). Checked every 5 s to avoid any overhead.
  static uint32_t lastApCheck = 0;
  uint32_t now = millis();
  if (now - lastApCheck >= 5000) {
    lastApCheck = now;
    if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
      Serial.println("[watchdog] AP down — restarting softAP");
      WiFi.softAP(ssid, password);
      WiFi.setSleep(false);
      esp_wifi_set_ps(WIFI_PS_NONE);
    }
  }
}
