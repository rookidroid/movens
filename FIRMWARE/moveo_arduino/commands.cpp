#include "commands.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "calibration.h"
#include "moveo_config.h"
#include "joints.h"

static QueueHandle_t cmdQueue;

void setupCommandQueue() {
  // Depth 20 — more than enough for burst button presses
  cmdQueue = xQueueCreate(20, sizeof(Command));
}

void enqueueCommand(CmdType type, int joint, int32_t val1, int32_t val2) {
  Command cmd = {type, joint, val1, val2};
  xQueueSend(cmdQueue, &cmd, 0);
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
          s->moveTo(clampSteps(getCal(cmd.joint), base + cmd.val1));
        }
        break;
      }

      case CMD_MOVETO: {
        FastAccelStepper* s = stepperByIndex(cmd.joint);
        if (s) s->moveTo(clampSteps(getCal(cmd.joint), cmd.val1));
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
          if (cmd.val1 > 0) s->setSpeedInHz(cmd.val1);
          if (cmd.val2 > 0) s->setAcceleration(cmd.val2);
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
          if (s) s->moveTo(clampSteps(getCal(i), 0));
        }
        break;

      case CMD_SERVO:
        writeServo((int)cmd.val1);
        break;
    }
  }
}
