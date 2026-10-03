#include "calibration.h"

#include <Preferences.h>

// Fallback when NVS is empty. Paste the initializer printed on Serial
// after a calibration save here to make it the new default.
static const JointCal DEFAULT_CAL[6] = {
  {0, 0, -180, 180, false},  // (unused)
  {0, 0, -180, 180, false},  // J1
  {0, 0, -180, 180, false},  // J2
  {0, 0, -180, 180, false},  // J3
  {0, 0, -180, 180, false},  // J4
  {0, 0, -180, 180, false},  // J5
};

static JointCal cal[6];  // index 1-5 used
static portMUX_TYPE calMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool dirty = false;  // set by updateCal(), cleared by saveCal()
static Preferences prefs;

JointCal getCal(int j) {
  portENTER_CRITICAL(&calMux);
  JointCal c = cal[j];
  portEXIT_CRITICAL(&calMux);
  return c;
}

void updateCal(int j, float spd, float home, float minDeg, float maxDeg, float limits) {
  portENTER_CRITICAL(&calMux);
  JointCal& c = cal[j];
  if (isfinite(spd))    c.spd    = spd;
  if (isfinite(home))   c.home   = home;
  if (isfinite(minDeg)) c.minDeg = minDeg;
  if (isfinite(maxDeg)) c.maxDeg = maxDeg;
  if (isfinite(limits)) c.limits = limits != 0;
  if (c.minDeg > c.maxDeg) { float t = c.minDeg; c.minDeg = c.maxDeg; c.maxDeg = t; }
  dirty = true;
  portEXIT_CRITICAL(&calMux);
}

bool isCalibrated(const JointCal& c) {
  return isfinite(c.spd) && fabsf(c.spd) > 1e-6f;
}

int32_t degToSteps(const JointCal& c, float deg) {
  return (int32_t)lroundf((deg - c.home) * c.spd);
}

float stepsToDeg(const JointCal& c, int32_t steps) {
  return c.home + steps / c.spd;
}

int32_t clampSteps(const JointCal& c, int32_t target) {
  if (!isCalibrated(c) || !c.limits) return target;
  int32_t a = degToSteps(c, c.minDeg);
  int32_t b = degToSteps(c, c.maxDeg);
  int32_t lo = a < b ? a : b;
  int32_t hi = a < b ? b : a;
  return constrain(target, lo, hi);
}

void loadCal() {
  memcpy(cal, DEFAULT_CAL, sizeof(cal));
  if (prefs.begin("moveo", true)) {
    if (prefs.getBytesLength("cal") == sizeof(cal)) {
      prefs.getBytes("cal", cal, sizeof(cal));
    }
    prefs.end();
  }
}

bool calDirty() {
  return dirty;
}

void saveCal() {
  JointCal copy[6];
  portENTER_CRITICAL(&calMux);
  memcpy(copy, cal, sizeof(cal));
  dirty = false;
  portEXIT_CRITICAL(&calMux);

  prefs.begin("moveo", false);
  prefs.putBytes("cal", copy, sizeof(copy));
  prefs.end();

  Serial.println("[calib] saved. DEFAULT_CAL initializer:");
  Serial.println("  {0, 0, -180, 180, false},  // (unused)");
  for (int j = 1; j <= 5; j++) {
    Serial.printf("  {%.5f, %.2f, %.2f, %.2f, %s},  // J%d\n",
                  copy[j].spd, copy[j].home, copy[j].minDeg, copy[j].maxDeg,
                  copy[j].limits ? "true" : "false", j);
  }
}
