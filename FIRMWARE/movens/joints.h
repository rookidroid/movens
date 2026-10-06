#pragma once

#include <Arduino.h>
#include "FastAccelStepper.h"

/* ─────────────────────────────────────────────
   Joint hardware: J1-J5 steppers + hand servo
   ───────────────────────────────────────────── */

// Last commanded servo pulse width (µs), readable from any core
extern volatile int servo_us;

void setupJoints();

// FastAccelStepper* for joint 1-5, nullptr otherwise
FastAccelStepper* stepperByIndex(int idx);

bool anyStepperRunning();

void writeServo(int us);
