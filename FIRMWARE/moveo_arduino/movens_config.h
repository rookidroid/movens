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

/* ── WiFi ──
   At boot the robot joins the network saved from the web UI (Network page).
   If none is saved, or it can't connect within WIFI_CONNECT_TIMEOUT_MS, it
   starts its own access point instead (http://192.168.4.1). */
#ifndef APSSID
  #define APSSID "movens"
  #define APPSK  "movens_1234"
#endif
#define WIFI_HOSTNAME           "movens"  // also mDNS: http://movens.local
#define WIFI_CONNECT_TIMEOUT_MS 15000

/* ── Arm geometry for kinematics (mm) ──
   NOMINAL values: measure your arm and update them.
   Frame: origin on the J1 axis at the base mounting surface, +Z up,
   +X forward at J1 = 0. Joint angles (deg) must follow these conventions,
   so calibrate each joint's home / direction to match:
     J1  yaw about +Z. 0 = arm faces +X, + = counter-clockwise seen from above
     J2  shoulder.     0 = upper arm vertical, + = tilts forward (toward the reach)
     J3  elbow.        0 = forearm in line with upper arm, + = bends forward
     J4  wrist roll.   0 = J5 axis parallel to J2 axis, + = right-handed about the forearm
     J5  wrist pitch.  0 = gripper in line with forearm, + = bends forward (at J4 = 0)
   All zero = arm pointing straight up. */
#define KIN_D1 232.0f  // base -> J2 axis height
#define KIN_A1   0.0f  // forward offset of J2 axis from J1 axis
#define KIN_A2 221.0f  // J2 axis -> J3 axis
#define KIN_D4 223.0f  // J3 axis -> J5 axis, along the forearm
#define KIN_A3   0.0f  // forearm axis offset from J3 axis (+ = forward of the forearm)
#define KIN_D6 175.0f  // J5 axis -> tool point (fingertip centre)
