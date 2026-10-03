#pragma once

/* ─────────────────────────────────────────────
   Hardware and network configuration
   ───────────────────────────────────────────── */

/* ── Joint stepper pins ── */
#define J1_DIR  13
#define J1_STEP 12

#define J2_DIR  14
#define J2_STEP 27

#define J3_DIR  26
#define J3_STEP 25

#define J4_DIR  33
#define J4_STEP 32

#define J5_DIR  15
#define J5_STEP 2

#define NUM_STEPPERS 5  // joints are numbered 1..NUM_STEPPERS

/* ── Stepper defaults ── */
#define STEPPER_SPEED_HZ 3000  // steps/s
#define STEPPER_ACCEL    800   // steps/s²

/* ── Hand servo ── */
#define servoPin 4
const int SERVO_MID = 1500;
const int SERVO_MIN = 700;
const int SERVO_MAX = 2300;

/* ── WiFi (Access Point) ── */
#ifndef APSSID
  #define APSSID "moveo"
  #define APPSK  "moveo_1234"
#endif
