#include "commands.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "calibration.h"
#include "moveo_config.h"
#include "joints.h"

static QueueHandle_t cmdQueue;

// Speed / accel: the saved motion profile at boot, then whatever CMD_CONFIG
// sets. CMD_MOVESYNC scales them per move, so
// every other move re-applies these. They can't be restored right after a
// sync moveTo(): the stepper task reads them asynchronously.
static uint32_t cfgSpeed[NUM_STEPPERS + 1];
static uint32_t cfgAccel[NUM_STEPPERS + 1];

void setupCommandQueue() {
  // Depth 20 — more than enough for burst button presses
  cmdQueue = xQueueCreate(20, sizeof(Command));
  for (int i = 1; i <= NUM_STEPPERS; i++) {
    JointMotion m = getMotion(i);  // loadCal() has run
    cfgSpeed[i] = m.speed;
    cfgAccel[i] = m.accel;
  }
}

uint32_t jointSpeed(int joint) { return cfgSpeed[joint]; }
uint32_t jointAccel(int joint) { return cfgAccel[joint]; }

void enqueueCommand(CmdType type, int joint, int32_t val1, int32_t val2) {
  Command cmd = {type, joint, val1, val2};
  xQueueSend(cmdQueue, &cmd, 0);
}

void enqueueMoveSync(const int32_t targets[NUM_STEPPERS], float speedScale) {
  Command cmd = {CMD_MOVESYNC};
  if (!isfinite(speedScale)) speedScale = 1.0f;
  cmd.val1 = constrain(lroundf(speedScale * 1000.0f), 10L, 1000L);  // per mille
  memcpy(cmd.targets, targets, sizeof(cmd.targets));
  xQueueSend(cmdQueue, &cmd, 0);
}

// moveTo() with the joint's configured speed / accel
static void moveJoint(int j, FastAccelStepper* s, int32_t target) {
  s->setSpeedInHz(cfgSpeed[j]);
  s->setAcceleration(cfgAccel[j]);
  s->moveTo(target);
}

// Duration of a trapezoidal / triangular move from rest
static float moveTime(float steps, float v, float a) {
  if (steps <= 0) return 0;
  return steps >= v * v / a ? steps / v + v / a : 2.0f * sqrtf(steps / a);
}

// Scaling a joint's speed by k and accel by k^2 stretches its move time by 1/k,
// so pick k = t_j / t_slowest for each joint and they all finish together.
// scale (0-1] slows every joint by the same factor on top of that.
static void moveSync(const int32_t targets[NUM_STEPPERS], float scale) {
  int32_t target[NUM_STEPPERS + 1];
  float   t[NUM_STEPPERS + 1] = {0};
  float   tMax = 0;
  for (int j = 1; j <= NUM_STEPPERS; j++) {
    FastAccelStepper* s = stepperByIndex(j);
    if (!s) continue;
    target[j] = clampSteps(getCal(j), targets[j - 1]);
    t[j] = moveTime(labs(target[j] - s->getCurrentPosition()), cfgSpeed[j], cfgAccel[j]);
    if (t[j] > tMax) tMax = t[j];
  }
  for (int j = 1; j <= NUM_STEPPERS; j++) {
    FastAccelStepper* s = stepperByIndex(j);
    if (!s) continue;
    float k = (tMax > 0 && t[j] > 0 ? t[j] / tMax : 1.0f) * scale;
    s->setSpeedInMilliHz(max(1UL, (unsigned long)lroundf(cfgSpeed[j] * 1000.0f * k)));
    s->setAcceleration(max(1L, lroundf(cfgAccel[j] * k * k)));
    s->moveTo(target[j]);
  }
}

void processCommands() {
  Command cmd;
  while (xQueueReceive(cmdQueue, &cmd, 0) == pdTRUE) {
    switch (cmd.type) {

      case CMD_MOVE: {
        // Relative to the running target (or current position if idle),
        // then clamped to the soft limits.
        FastAccelStepper* s = stepperByIndex(cmd.joint);
        if (s) {
          int32_t base = s->isRunning() ? s->targetPos() : s->getCurrentPosition();
          moveJoint(cmd.joint, s, clampSteps(getCal(cmd.joint), base + cmd.val1));
        }
        break;
      }

      case CMD_MOVETO: {
        FastAccelStepper* s = stepperByIndex(cmd.joint);
        if (s) moveJoint(cmd.joint, s, clampSteps(getCal(cmd.joint), cmd.val1));
        break;
      }

      case CMD_SETPOS: {
        FastAccelStepper* s = stepperByIndex(cmd.joint);
        if (s && !s->isRunning()) s->setCurrentPosition(cmd.val1);
        break;
      }

      case CMD_CONFIG: {
        FastAccelStepper* s = stepperByIndex(cmd.joint);
        if (s) {
          if (cmd.val1 > 0) s->setSpeedInHz(cfgSpeed[cmd.joint] = cmd.val1);
          if (cmd.val2 > 0) s->setAcceleration(cfgAccel[cmd.joint] = cmd.val2);
        }
        break;
      }

      case CMD_STOP:
        for (int i = 1; i <= NUM_STEPPERS; i++) {
          FastAccelStepper* s = stepperByIndex(i);
          if (s) s->stopMove();
        }
        break;

      case CMD_HOME:
        for (int i = 1; i <= NUM_STEPPERS; i++) {
          FastAccelStepper* s = stepperByIndex(i);
          if (s) moveJoint(i, s, clampSteps(getCal(i), 0));
        }
        break;

      case CMD_MOVESYNC:
        moveSync(cmd.targets, cmd.val1 / 1000.0f);
        break;

      case CMD_SERVO:
        writeServo((int)cmd.val1);
        break;
    }
  }
}
