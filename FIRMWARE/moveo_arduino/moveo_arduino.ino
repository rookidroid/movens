/**

  Moveo

  - Copyright (C) 2024 - PRESENT  rookidroid.com
  - E-mail: info@rookidroid.com
  - Website: https://rookidroid.com/

                        **
                       ****
                        **
                        **
                        **
                        **

        **********************************
      **************************************
     ****************************************
     ********      ************      ********
     *******        **********        *******
     *******        **********        *******
     ********      ************      ********
     ****************************************
     ****************************************
     ****************************************
     ****************************************


            **************************

                ******************

*/

/*
   Module layout
     moveo_config.h    pins, servo range, WiFi credentials
     joints.*          stepper engine, J1-J5 steppers, hand servo
     calibration.*     steps <-> degrees, soft limits, NVS persistence
     kinematics.*      forward / inverse kinematics (tool pose <-> joint angles)
     commands.*        FreeRTOS queue bridging web callbacks (Core 0) to loop() (Core 1)
     connectivity.*    WiFi station / access point fallback, saved network, OTA
     web_server.*      async HTTP routes / REST API
     web_*.h           embedded HTML / CSS / JS
     json_util.*       minimal JSON number parsing
*/

#include "calibration.h"
#include "commands.h"
#include "joints.h"
#include "connectivity.h"
#include "web_server.h"

void setup() {
  Serial.begin(115200);

  loadCal();
  setupWiFi();
  setupOTA();
  setupJoints();
  setupCommandQueue();
  setupWebServer();
}

/* ─────────────────────────────────────────────
   loop()  — runs on Core 1
   Drains the command queue so all motor / servo
   API calls happen here, away from Core 0 lwIP.
   ───────────────────────────────────────────── */
void loop() {
  handleNetwork();

  processCommands();

  // Persist calibration once all joints are idle
  if (calDirty() && !anyStepperRunning()) saveCal();

  // Yield to FreeRTOS scheduler so the WiFi task (Core 0) and other
  // system tasks get CPU time every loop iteration.
  vTaskDelay(1);
}
