#pragma once

#include <stdint.h>

#include "GreenGuardCore.h"

namespace greenguard {

struct ActuatorOutput {
  uint16_t rpwm = 0;
  uint16_t lpwm = 0;
  bool enableHigh = false;
};

inline ActuatorOutput actuatorOutput(Drive drive, bool dryRun, bool controlEnable,
                                     bool deployUsesRpwm, uint16_t duty) {
  ActuatorOutput output;
  if (dryRun || drive == Drive::OFF) return output;
  output.enableHigh = controlEnable;
  const bool rpwm = (drive == Drive::DEPLOY) == deployUsesRpwm;
  if (rpwm) output.rpwm = duty;
  else output.lpwm = duty;
  return output;
}

}  // namespace greenguard
