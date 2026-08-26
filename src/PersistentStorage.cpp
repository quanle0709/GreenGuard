#include "PersistentStorage.h"

#include <LittleFS.h>
#include <math.h>

#include "HardwareConfig.h"

namespace {
constexpr uint8_t STORAGE_SCHEMA = 2;

bool validMode(uint8_t value) { return value <= static_cast<uint8_t>(greenguard::Mode::MANUAL); }
bool validShield(uint8_t value) { return value <= static_cast<uint8_t>(greenguard::ShieldState::STOPPED_PARTIAL); }
bool validConfidence(uint8_t value) { return value <= static_cast<uint8_t>(greenguard::PositionConfidence::LIMIT_CONFIRMED); }
bool validFault(uint8_t value) { return value <= static_cast<uint8_t>(greenguard::Fault::STORAGE_ERROR); }
}  // namespace

bool PersistentStorage::begin() { return LittleFS.begin(); }

bool PersistentStorage::atomicWrite(const char* path, const JsonDocument& document) {
  const String temporary = String(path) + ".tmp";
  const String backup = String(path) + ".bak";
  LittleFS.remove(temporary);
  File file = LittleFS.open(temporary, "w");
  if (!file) return false;
  const size_t written = serializeJson(document, file);
  file.flush();
  file.close();
  if (written == 0) {
    LittleFS.remove(temporary);
    return false;
  }

  JsonDocument check;
  file = LittleFS.open(temporary, "r");
  const bool readable = file && deserializeJson(check, file) == DeserializationError::Ok;
  file.close();
  if (!readable) {
    LittleFS.remove(temporary);
    return false;
  }

  LittleFS.remove(backup);
  const bool hadOriginal = LittleFS.exists(path);
  if (hadOriginal && !LittleFS.rename(path, backup)) {
    LittleFS.remove(temporary);
    return false;
  }
  if (!LittleFS.rename(temporary, path)) {
    if (hadOriginal) LittleFS.rename(backup, path);
    LittleFS.remove(temporary);
    return false;
  }
  LittleFS.remove(backup);
  return true;
}

LoadResult PersistentStorage::loadConfig(DeviceConfig& config) {
  if (!LittleFS.exists("/config.json")) return LoadResult::MISSING;
  File file = LittleFS.open("/config.json", "r");
  JsonDocument document;
  if (!file || deserializeJson(document, file) != DeserializationError::Ok) {
    file.close();
    return LoadResult::INVALID;
  }
  file.close();
  if ((document["schema"] | 0) != STORAGE_SCHEMA) return LoadResult::INVALID;

  DeviceConfig candidate;
  candidate.controller.wetConfirmMs = document["wetConfirmMs"] | candidate.controller.wetConfirmMs;
  candidate.controller.dryConfirmMs = document["dryConfirmMs"] | candidate.controller.dryConfirmMs;
  candidate.controller.directionDeadTimeMs = document["directionDeadTimeMs"] | candidate.controller.directionDeadTimeMs;
  candidate.controller.inferredFullTravelMs = document["inferredFullTravelMs"] | candidate.controller.inferredFullTravelMs;
  candidate.controller.motorMaxRuntimeMs = document["motorMaxRuntimeMs"] | candidate.controller.motorMaxRuntimeMs;
  candidate.controller.useLimitSwitches = hardware::USE_LIMIT_SWITCHES;
  candidate.rainActiveLow = document["rainActiveLow"] | candidate.rainActiveLow;
  candidate.deployUsesRpwm = document["deployUsesRpwm"] | candidate.deployUsesRpwm;
  candidate.motorPwm = document["motorPwm"] | candidate.motorPwm;
  if (!validDeviceConfig(candidate)) return LoadResult::INVALID;
  config = candidate;
  return LoadResult::VALID;
}

bool PersistentStorage::saveConfig(const DeviceConfig& config) {
  if (!validDeviceConfig(config)) return false;
  JsonDocument document;
  document["schema"] = STORAGE_SCHEMA;
  document["wetConfirmMs"] = config.controller.wetConfirmMs;
  document["dryConfirmMs"] = config.controller.dryConfirmMs;
  document["directionDeadTimeMs"] = config.controller.directionDeadTimeMs;
  document["inferredFullTravelMs"] = config.controller.inferredFullTravelMs;
  document["motorMaxRuntimeMs"] = config.controller.motorMaxRuntimeMs;
  document["rainActiveLow"] = config.rainActiveLow;
  document["deployUsesRpwm"] = config.deployUsesRpwm;
  document["motorPwm"] = config.motorPwm;
  return atomicWrite("/config.json", document);
}

LoadResult PersistentStorage::loadSnapshot(greenguard::Snapshot& snapshot) {
  if (!LittleFS.exists("/state.json")) return LoadResult::MISSING;
  File file = LittleFS.open("/state.json", "r");
  JsonDocument document;
  if (!file || deserializeJson(document, file) != DeserializationError::Ok) {
    file.close();
    return LoadResult::INVALID;
  }
  file.close();
  if ((document["schema"] | 0) != STORAGE_SCHEMA) return LoadResult::INVALID;

  const uint8_t mode = document["mode"] | 255;
  const uint8_t shield = document["shield"] | 255;
  const uint8_t confidence = document["confidence"] | 255;
  const uint8_t fault = document["fault"] | 255;
  const float estimate = document["estimatedPositionPct"] | -1.0f;
  if (!validMode(mode) || !validShield(shield) || !validConfidence(confidence) || !validFault(fault) ||
      !isfinite(estimate) || estimate < 0.0f || estimate > 100.0f) return LoadResult::INVALID;

  snapshot.mode = static_cast<greenguard::Mode>(mode);
  snapshot.shield = static_cast<greenguard::ShieldState>(shield);
  snapshot.confidence = static_cast<greenguard::PositionConfidence>(confidence);
  snapshot.estimatedPositionPct = estimate;
  snapshot.estimateAvailable = document["estimateAvailable"] | false;
  snapshot.stopLatched = document["stopLatched"] | false;
  snapshot.fault = static_cast<greenguard::Fault>(fault);
  snapshot.movementActive = document["movementActive"] | false;
  snapshot.rainEventCount = document["rainEventCount"] | 0;
  return LoadResult::VALID;
}

bool PersistentStorage::saveSnapshot(const greenguard::Snapshot& snapshot) {
  JsonDocument document;
  document["schema"] = STORAGE_SCHEMA;
  document["mode"] = static_cast<uint8_t>(snapshot.mode);
  document["shield"] = static_cast<uint8_t>(snapshot.shield);
  document["confidence"] = static_cast<uint8_t>(snapshot.confidence);
  document["estimatedPositionPct"] = snapshot.estimatedPositionPct;
  document["estimateAvailable"] = snapshot.estimateAvailable;
  document["stopLatched"] = snapshot.stopLatched;
  document["fault"] = static_cast<uint8_t>(snapshot.fault);
  document["movementActive"] = snapshot.movementActive;
  document["rainEventCount"] = snapshot.rainEventCount;
  return atomicWrite("/state.json", document);
}
