#pragma once
#include "types.h"
#include <ArduinoJson.h>
class PersistentStorage {
 public:
  bool begin(); bool loadConfig(AppConfig&); bool saveConfig(const AppConfig&);
  bool loadState(PersistentState&); bool saveState(const PersistentState&);
  static bool validateConfig(const AppConfig&);
 private: bool atomicWrite(const char*, const JsonDocument&);
};
