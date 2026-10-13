#include "joints.h"

#include "calibration.h"
#include "movens_config.h"

static FastAccelStepperEngine engine = FastAccelStepperEngine();

static const uint8_t STEP_PINS[NUM_STEPPERS + 1] = {0, J1_STEP, J2_STEP, J3_STEP, J4_STEP, J5_STEP};
static const uint8_t DIR_PINS[NUM_STEPPERS + 1]  = {0, J1_DIR,  J2_DIR,  J3_DIR,  J4_DIR,  J5_DIR};

static FastAccelStepper* steppers[NUM_STEPPERS + 1] = {nullptr};  // index 1-5 used

// The hand servo runs on LEDC. ESP32Servo pulls in the legacy MCPWM driver on
// the ESP32-S3, which can't coexist with the MCPWM driver FastAccelStepper uses.
static const uint32_t SERVO_FREQ_HZ   = 50;
static const uint8_t  SERVO_PWM_BITS  = 14;  // highest LEDC resolution the S3 has
static const uint32_t SERVO_PERIOD_US = 1000000UL / SERVO_FREQ_HZ;

volatile int servo_us = SERVO_MID;

// Pick the step generator explicitly: RMT, then MCPWM/PCNT. Left to choose,
// FastAccelStepper puts the 5th stepper on its I2S driver on the ESP32-S3
// (only 4 RMT channels). That driver's interrupt always runs and calls code
// held in flash, so the chip crashes on every flash write (NVS saves).
static FastAccelStepper* connectStepper(uint8_t stepPin) {
  FastAccelStepper* s = engine.stepperConnectToPin(stepPin, DRIVER_RMT);
  if (!s) s = engine.stepperConnectToPin(stepPin, DRIVER_MCPWM_PCNT);
  return s;
}

void setupJoints() {
  // ── GPIO ─────────────────────────────────────
  for (int i = 1; i <= NUM_STEPPERS; i++) {
    pinMode(DIR_PINS[i],  OUTPUT);
    pinMode(STEP_PINS[i], OUTPUT);
  }

  // ── Servo ─────────────────────────────────────
  ledcAttach(servoPin, SERVO_FREQ_HZ, SERVO_PWM_BITS);
  writeServo(SERVO_MID);

  // ── Steppers ──────────────────────────────────
  engine.init();

  for (int i = 1; i <= NUM_STEPPERS; i++) {
    FastAccelStepper* s = connectStepper(STEP_PINS[i]);
    if (s) {
      s->setDirectionPin(DIR_PINS[i]);
      JointMotion m = getMotion(i);  // loadCal() has run
      s->setSpeedInHz(m.speed);
      s->setAcceleration(m.accel);
    }
    steppers[i] = s;
  }
}

FastAccelStepper* stepperByIndex(int idx) {
  if (idx < 1 || idx > NUM_STEPPERS) return nullptr;
  return steppers[idx];
}

bool anyStepperRunning() {
  for (int i = 1; i <= NUM_STEPPERS; i++) {
    FastAccelStepper* s = steppers[i];
    if (s && s->isRunning()) return true;
  }
  return false;
}

void writeServo(int us) {
  us = constrain(us, SERVO_MIN, SERVO_MAX);
  ledcWrite(servoPin, ((uint32_t)us << SERVO_PWM_BITS) / SERVO_PERIOD_US);
}
