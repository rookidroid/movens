#include "web_server.h"

#include <Arduino.h>

/** Async Web Server
 *  Install via Arduino Library Manager:
 *    - "ESPAsyncWebServer" by ESP32Async
 *    - "AsyncTCP" by ESP32Async  ← required dependency
 */
#include <ESPAsyncWebServer.h>

#include "calibration.h"
#include "commands.h"
#include "moveo_config.h"
#include "joints.h"
#include "json_util.h"

/* ── Embedded web assets (PROGMEM) ──
   /app.css, /app.js  shared by both pages
   /                  joint control page
   /calibrate         joint calibration page */
#include "web_app_css.h"
#include "web_app_js.h"
#include "web_calib_html.h"
#include "web_index_html.h"

static AsyncWebServer server(80);

/* ─────────────────────────────────────────────
   Helper: add CORS headers to every response
   ───────────────────────────────────────────── */
static void addCors(AsyncWebServerResponse* resp) {
  resp->addHeader("Access-Control-Allow-Origin",  "*");
  resp->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

static bool validJoint(int j) {
  return j >= 1 && j <= NUM_STEPPERS;
}

static void sendOk(AsyncWebServerRequest* request) {
  request->send(200, "application/json", "{\"ok\":true}");
}

static void sendJointNotFound(AsyncWebServerRequest* request) {
  request->send(404, "application/json", "{\"error\":\"joint not found\"}");
}

/* ─────────────────────────────────────────────
   Async REST Route Handlers
   All callbacks run in the lwIP/WiFi task (Core 0)
   — never block here, never call delay().
   Motor calls are enqueued and executed in loop() on Core 1.
   ───────────────────────────────────────────── */

// Serve an embedded PROGMEM asset (no-cache so OTA updates show up immediately)
static void sendAsset(AsyncWebServerRequest* request, const char* type, const char* data) {
  AsyncWebServerResponse* resp = request->beginResponse_P(200, type, data);
  resp->addHeader("Cache-Control", "no-cache");
  request->send(resp);
}

// GET /  →  serve the control page
static void handleRoot(AsyncWebServerRequest* request) {
  sendAsset(request, "text/html", INDEX_HTML);
}

// GET /calibrate  →  serve the calibration page
static void handleCalibratePage(AsyncWebServerRequest* request) {
  sendAsset(request, "text/html", CALIB_HTML);
}

// GET /app.css, /app.js  →  assets shared by both pages
static void handleAppCss(AsyncWebServerRequest* request) {
  sendAsset(request, "text/css; charset=utf-8", APP_CSS);
}

static void handleAppJs(AsyncWebServerRequest* request) {
  sendAsset(request, "application/javascript; charset=utf-8", APP_JS);
}

// GET /status  →  JSON with current positions
//   jN: steps, aN: degrees (null if uncalibrated), mN: 1 while moving
static void handleStatus(AsyncWebServerRequest* request) {
  String json = "{";
  for (int i = 1; i <= NUM_STEPPERS; i++) {
    FastAccelStepper* s = stepperByIndex(i);
    long pos = s ? s->getCurrentPosition() : 0;
    JointCal c = getCal(i);
    json += "\"j" + String(i) + "\":" + String(pos);
    json += ",\"a" + String(i) + "\":" + (isCalibrated(c) ? String(stepsToDeg(c, pos), 2) : String("null"));
    json += ",\"m" + String(i) + "\":" + String((s && s->isRunning()) ? 1 : 0);
    if (i < NUM_STEPPERS) json += ",";
  }
  json += ",\"servo\":" + String(servo_us) + "}";
  AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", json);
  addCors(resp);
  request->send(resp);
}

// POST /move  →  body {joint, steps}
static void handleMove(AsyncWebServerRequest* request,
                       uint8_t* data, size_t len, size_t /*index*/, size_t /*total*/) {
  String body  = String((char*)data, len);
  int jIdx     = jsonInt(body, "joint");
  int steps    = jsonInt(body, "steps");
  if (!validJoint(jIdx)) { sendJointNotFound(request); return; }
  enqueueCommand(CMD_MOVE, jIdx, steps);
  sendOk(request);
}

// POST /moveto  →  body {joint, pos}
static void handleMoveTo(AsyncWebServerRequest* request,
                         uint8_t* data, size_t len, size_t /*index*/, size_t /*total*/) {
  String body  = String((char*)data, len);
  int jIdx     = jsonInt(body, "joint");
  int pos      = jsonInt(body, "pos");
  if (!validJoint(jIdx)) { sendJointNotFound(request); return; }
  enqueueCommand(CMD_MOVETO, jIdx, pos);
  sendOk(request);
}

// POST /config  →  body {joint, speed, accel}
static void handleConfig(AsyncWebServerRequest* request,
                         uint8_t* data, size_t len, size_t /*index*/, size_t /*total*/) {
  String body  = String((char*)data, len);
  int jIdx     = jsonInt(body, "joint");
  int speed    = jsonInt(body, "speed");
  int accel    = jsonInt(body, "accel");
  if (!validJoint(jIdx)) { sendJointNotFound(request); return; }
  enqueueCommand(CMD_CONFIG, jIdx, speed, accel);
  sendOk(request);
}

// POST /stop  →  stop all motors immediately
static void handleStop(AsyncWebServerRequest* request) {
  enqueueCommand(CMD_STOP);
  sendOk(request);
}

// POST /home  →  move all steppers to position 0
static void handleHome(AsyncWebServerRequest* request) {
  enqueueCommand(CMD_HOME);
  sendOk(request);
}

// POST /servo  →  body {us}
static void handleServo(AsyncWebServerRequest* request,
                        uint8_t* data, size_t len, size_t /*index*/, size_t /*total*/) {
  String body  = String((char*)data, len);
  int us       = constrain(jsonInt(body, "us"), SERVO_MIN, SERVO_MAX);
  servo_us     = us;  // update the readable state immediately
  enqueueCommand(CMD_SERVO, 0, us);
  sendOk(request);
}

// GET /calib  →  calibration of all joints
static void handleCalibGet(AsyncWebServerRequest* request) {
  String json = "{";
  for (int j = 1; j <= NUM_STEPPERS; j++) {
    JointCal c = getCal(j);
    json += "\"j" + String(j) + "\":{";
    json += "\"spd\":"     + String(c.spd, 5);
    json += ",\"home\":"   + String(c.home, 2);
    json += ",\"min\":"    + String(c.minDeg, 2);
    json += ",\"max\":"    + String(c.maxDeg, 2);
    json += ",\"limits\":" + String(c.limits ? "true" : "false");
    json += "}";
    if (j < NUM_STEPPERS) json += ",";
  }
  json += "}";
  AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", json);
  addCors(resp);
  request->send(resp);
}

// POST /calib  →  body {joint, spd?, home?, min?, max?, limits?}
// Omitted fields keep their current value. Saved to NVS from loop().
static void handleCalibSet(AsyncWebServerRequest* request,
                           uint8_t* data, size_t len, size_t /*index*/, size_t /*total*/) {
  String body  = String((char*)data, len);
  int jIdx     = jsonInt(body, "joint");
  if (!validJoint(jIdx)) { sendJointNotFound(request); return; }
  updateCal(jIdx,
            jsonFloat(body, "spd"),
            jsonFloat(body, "home"),
            jsonFloat(body, "min"),
            jsonFloat(body, "max"),
            jsonFloat(body, "limits"));
  sendOk(request);
}

// POST /moveangle  →  body {joint, deg}
static void handleMoveAngle(AsyncWebServerRequest* request,
                            uint8_t* data, size_t len, size_t /*index*/, size_t /*total*/) {
  String body  = String((char*)data, len);
  int jIdx     = jsonInt(body, "joint");
  float deg    = jsonFloat(body, "deg");
  if (!validJoint(jIdx)) { sendJointNotFound(request); return; }
  if (!isfinite(deg))    { request->send(400, "application/json", "{\"error\":\"missing deg\"}"); return; }
  JointCal c = getCal(jIdx);
  if (!isCalibrated(c))  { request->send(409, "application/json", "{\"error\":\"joint not calibrated\"}"); return; }
  enqueueCommand(CMD_MOVETO, jIdx, degToSteps(c, deg));  // soft limits applied in loop()
  sendOk(request);
}

// POST /setpos  →  body {joint, deg}
// Declare "the joint is at <deg> right now" without moving it.
// Uncalibrated joints only accept deg == home (i.e. zero at the alignment pose).
static void handleSetPos(AsyncWebServerRequest* request,
                         uint8_t* data, size_t len, size_t /*index*/, size_t /*total*/) {
  String body  = String((char*)data, len);
  int jIdx     = jsonInt(body, "joint");
  float deg    = jsonFloat(body, "deg");
  if (!validJoint(jIdx))   { sendJointNotFound(request); return; }
  if (!isfinite(deg))      { request->send(400, "application/json", "{\"error\":\"missing deg\"}"); return; }
  FastAccelStepper* s = stepperByIndex(jIdx);
  if (s && s->isRunning()) { request->send(409, "application/json", "{\"error\":\"joint is moving\"}"); return; }
  JointCal c = getCal(jIdx);
  if (!isCalibrated(c) && fabsf(deg - c.home) > 1e-3f) {
    request->send(409, "application/json", "{\"error\":\"joint not calibrated\"}");
    return;
  }
  enqueueCommand(CMD_SETPOS, jIdx, isCalibrated(c) ? degToSteps(c, deg) : 0);
  sendOk(request);
}

// Register a POST route whose handler needs the request body.
// The ACK is sent inside the body handler, so the request handler is empty.
static void onPostBody(const char* path, ArBodyHandlerFunction bodyHandler) {
  server.on(path, HTTP_POST, [](AsyncWebServerRequest*){ }, NULL, bodyHandler);
}

void setupWebServer() {
  // GET endpoints
  server.on("/",          HTTP_GET, handleRoot);
  server.on("/calibrate", HTTP_GET, handleCalibratePage);
  server.on("/app.css",   HTTP_GET, handleAppCss);
  server.on("/app.js",    HTTP_GET, handleAppJs);
  server.on("/status",    HTTP_GET, handleStatus);
  server.on("/calib",     HTTP_GET, handleCalibGet);

  // POST endpoints without a body
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/home", HTTP_POST, handleHome);

  // POST endpoints with a JSON body
  onPostBody("/move",      handleMove);
  onPostBody("/moveto",    handleMoveTo);
  onPostBody("/config",    handleConfig);
  onPostBody("/servo",     handleServo);
  onPostBody("/calib",     handleCalibSet);
  onPostBody("/moveangle", handleMoveAngle);
  onPostBody("/setpos",    handleSetPos);

  // CORS pre-flight (OPTIONS) — reply 204 for all paths
  server.onNotFound([](AsyncWebServerRequest* request) {
    if (request->method() == HTTP_OPTIONS) {
      AsyncWebServerResponse* resp = request->beginResponse(204);
      addCors(resp);
      request->send(resp);
    } else {
      request->send(404, "text/plain", "Not found");
    }
  });

  server.begin();
  Serial.println("Async web server started on http://192.168.4.1");
}
