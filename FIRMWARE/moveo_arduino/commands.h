#pragma once

#include <Arduino.h>

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
  CMD_SETPOS   // joint + pos   (redefine current position, no motion)
};

struct Command {
  CmdType type;
  int     joint;  // 1-5 for steppers
  int32_t val1;   // steps / pos / speed / us
  int32_t val2;   // accel (CMD_CONFIG only)
};

void setupCommandQueue();

// Non-blocking; safe to call from web callbacks. Drops the command if the queue is full.
void enqueueCommand(CmdType type, int joint = 0, int32_t val1 = 0, int32_t val2 = 0);

// Drain the queue and execute every pending command. Call from loop() only.
void processCommands();
