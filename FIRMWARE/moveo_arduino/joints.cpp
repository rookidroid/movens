#include "joints.h"

#include <ESP32Servo.h>

#include "moveo_config.h"

static FastAccelStepperEngine engine = FastAccelStepperEngine();

static const uint8_t STEP_PINS[NUM_STEPPERS + 1] = {0, J1_STEP, J2_STEP, J3_STEP, J4_STEP, J5_STEP};
static const uint8_t DIR_PINS[NUM_STEPPERS + 1]  = {0, J1_DIR,  J2_DIR,  J3_DIR,  J4_DIR,  J5_DIR};

static FastAccelStepper* steppers[NUM_STEPPERS + 1] = {nullptr};  // index 1-5 used

static Servo hand_servo;
volatile int servo_us = SERVO_MID;

void setupJoints() {
  // ── GPIO ─────────────────────────────────────
  for (int i = 1; i <= NUM_STEPPERS; i++) {
    pinMode(DIR_PINS[i],  OUTPUT);
    pinMode(STEP_PINS[i], OUTPUT);
  }

  // ── Servo ─────────────────────────────────────
  hand_servo.attach(servoPin, SERVO_MIN, SERVO_MAX);
  hand_servo.writeMicroseconds(SERVO_MID);

  // ── Steppers ──────────────────────────────────
  engine.init();

  for (int i = 1; i <= NUM_STEPPERS; i++) {
    FastAccelStepper* s = engine.stepperConnectToPin(STEP_PINS[i]);
    if (s) {
      s->setDirectionPin(DIR_PINS[i]);
      s->setSpeedInHz(STEPPER_SPEED_HZ);
      s->setAcceleration(STEPPER_ACCEL);
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
  hand_servo.writeMicroseconds(us);
}
