#pragma once

#include <stdint.h>

#include "GreenGuardCore.h"

struct DeviceConfig {
  greenguard::Config controller;
  bool rainActiveLow = true;
  bool deployUsesRpwm = true;
  uint16_t motorPwm = 820;
};

inline bool validDeviceConfig(const DeviceConfig& config) {
  return greenguard::Controller::validConfig(config.controller) && config.motorPwm <= 1023;
}
