#pragma once

#include <ArduinoJson.h>

#include "DeviceConfig.h"
#include "GreenGuardCore.h"

enum class LoadResult : uint8_t { MISSING = 0, VALID = 1, INVALID = 2 };

class PersistentStorage {
 public:
  bool begin();
  LoadResult loadConfig(DeviceConfig& config);
  bool saveConfig(const DeviceConfig& config);
  LoadResult loadSnapshot(greenguard::Snapshot& snapshot);
  bool saveSnapshot(const greenguard::Snapshot& snapshot);

 private:
  bool atomicWrite(const char* path, const JsonDocument& document);
};
