#pragma once

#include <Arduino.h>

#include "moveo_config.h"

/* ─────────────────────────────────────────────
   FreeRTOS Command Queue
   Web handlers (Core 0) post commands here;
   loop() (Core 1) drains and executes them.
   ───────────────────────────────────────────── */
enum CmdType : uint8_t {
  CMD_MOVE,    // joint + steps (relative)
  CMD_MOVETO,  // joint + pos   (absolute)
  CMD_CONFIG,  // joint + speed + accel
  CMD_STOP,    // stop all
  CMD_HOME,    // home all
  CMD_SERVO,   // servo us
  CMD_SETPOS,  // joint + pos   (redefine current position, no motion)
  CMD_MOVESYNC // targets for all joints (absolute), arriving together
};

struct Command {
  CmdType type;
  int     joint;  // 1-5 for steppers
  int32_t val1;   // steps / pos / speed / us
  int32_t val2;   // accel (CMD_CONFIG only)
  int32_t targets[NUM_STEPPERS];  // CMD_MOVESYNC only, index 0 = J1
};

void setupCommandQueue();

// Non-blocking; safe to call from web callbacks. Drops the command if the queue is full.
void enqueueCommand(CmdType type, int joint = 0, int32_t val1 = 0, int32_t val2 = 0);

// Move every joint to targets[0..NUM_STEPPERS-1] (absolute steps) so they
// start and finish together. Same queueing rules as enqueueCommand().
void enqueueMoveSync(const int32_t targets[NUM_STEPPERS]);

// Speed (steps/s) and acceleration (steps/s²) last set through CMD_CONFIG.
uint32_t jointSpeed(int joint);
uint32_t jointAccel(int joint);

// Drain the queue and execute every pending command. Call from loop() only.
void processCommands();
