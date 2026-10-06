#pragma once

#include <Arduino.h>

/* ─────────────────────────────────────────────
   Joint Calibration  (persisted in NVS)
     angle_deg = home + steps / spd
     steps     = (angle_deg - home) * spd
   Step 0 is the alignment pose at power-on.
   spd == 0 means the joint is not calibrated.
   ───────────────────────────────────────────── */
struct JointCal {
  float spd;     // steps per degree (signed: sign = direction)
  float home;    // angle (deg) of the alignment pose (step 0)
  float minDeg;  // soft limit
  float maxDeg;  // soft limit
  bool  limits;  // soft limits enabled
};

// Thread-safe copy of joint j's calibration (j = 1-5)
JointCal getCal(int j);

// Update joint j's calibration; non-finite arguments keep the current value.
// Marks the calibration dirty so loop() persists it.
void updateCal(int j, float spd, float home, float minDeg, float maxDeg, float limits);

bool    isCalibrated(const JointCal& c);
int32_t degToSteps(const JointCal& c, float deg);
float   stepsToDeg(const JointCal& c, int32_t steps);

// Clamp a target step position to the joint's soft limits (if enabled)
int32_t clampSteps(const JointCal& c, int32_t target);

/* Saved motion profile of a joint, applied at boot. Persisted with the
   calibration but stored separately so older calibrations still load. */
struct JointMotion {
  uint32_t speed;  // steps/s
  uint32_t accel;  // steps/s²
};

// Thread-safe copy of joint j's saved motion profile (j = 1-5)
JointMotion getMotion(int j);

// Update joint j's saved motion profile; 0 keeps the current value.
// Marks the calibration dirty so loop() persists it.
void updateMotion(int j, uint32_t speed, uint32_t accel);

void loadCal();

// True when updateCal() has changes not yet written by saveCal()
bool calDirty();

// Called from loop() only. NVS writes stall the flash cache,
// so this is deferred until no stepper is running.
void saveCal();
