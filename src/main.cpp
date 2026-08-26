#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <math.h>
#include <string.h>

#include "ActuatorOutput.h"
#include "DeviceConfig.h"
#include "GreenGuardCore.h"
#include "HardwareConfig.h"
#include "PersistentStorage.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

using namespace greenguard;

PersistentStorage storage;
DeviceConfig deviceConfig;
Controller controller;
ESP8266WebServer server(80);

bool filesystemReady = false;
bool mdnsStarted = false;
bool sensorRawWet = false;
uint8_t sensorDigitalLevel = HIGH;
uint32_t lastSensorSampleMs = 0;
uint32_t lastWifiAttemptMs = 0;
uint32_t lastDiagnosticMs = 0;
Snapshot lastPersistedSnapshot;
bool persistedSnapshotKnown = false;
ActuatorOutput lastActuatorOutput;

bool intervalElapsed(uint32_t now, uint32_t since, uint32_t interval) {
  return static_cast<uint32_t>(now - since) >= interval;
}

bool limitActive(uint8_t pin) {
  const bool levelHigh = digitalRead(pin) == HIGH;
  return hardware::LIMIT_ACTIVE_LOW ? !levelHigh : levelHigh;
}

Inputs readInputs(uint32_t nowMs) {
  if (intervalElapsed(nowMs, lastSensorSampleMs, hardware::SENSOR_SAMPLE_INTERVAL_MS)) {
    lastSensorSampleMs = nowMs;
    sensorDigitalLevel = digitalRead(hardware::RAIN_DO_PIN);
    sensorRawWet = deviceConfig.rainActiveLow ? sensorDigitalLevel == LOW : sensorDigitalLevel == HIGH;
  }
  Inputs inputs;
  inputs.nowMs = nowMs;
  inputs.rainWet = sensorRawWet;
  if (hardware::USE_LIMIT_SWITCHES) {
    inputs.retractedLimit = limitActive(hardware::RETRACTED_LIMIT_PIN);
    inputs.deployedLimit = limitActive(hardware::DEPLOYED_LIMIT_PIN);
  }
  return inputs;
}

void initializeActuatorPins() {
  analogWriteRange(1023);
  pinMode(hardware::BTS_RPWM_PIN, OUTPUT);
  pinMode(hardware::BTS_LPWM_PIN, OUTPUT);
  analogWrite(hardware::BTS_RPWM_PIN, 0);
  analogWrite(hardware::BTS_LPWM_PIN, 0);
  if (hardware::CONTROL_BTS_ENABLE) {
    pinMode(hardware::BTS_ENABLE_PIN, OUTPUT);
    digitalWrite(hardware::BTS_ENABLE_PIN, LOW);
  }
}

void applyActuatorOutput() {
  const ActuatorOutput output = actuatorOutput(controller.drive(), hardware::ACTUATOR_DRY_RUN,
      hardware::CONTROL_BTS_ENABLE, deviceConfig.deployUsesRpwm, deviceConfig.motorPwm);
  if (output.rpwm == lastActuatorOutput.rpwm && output.lpwm == lastActuatorOutput.lpwm &&
      output.enableHigh == lastActuatorOutput.enableHigh) return;

  // Always switch both PWM inputs off before changing direction or enable state.
  analogWrite(hardware::BTS_RPWM_PIN, 0);
  analogWrite(hardware::BTS_LPWM_PIN, 0);
  if (hardware::CONTROL_BTS_ENABLE) digitalWrite(hardware::BTS_ENABLE_PIN, LOW);
  if (output.rpwm > 0) analogWrite(hardware::BTS_RPWM_PIN, output.rpwm);
  if (output.lpwm > 0) analogWrite(hardware::BTS_LPWM_PIN, output.lpwm);
  if (hardware::CONTROL_BTS_ENABLE && output.enableHigh) digitalWrite(hardware::BTS_ENABLE_PIN, HIGH);
  lastActuatorOutput = output;
}

bool snapshotsDiffer(const Snapshot& left, const Snapshot& right) {
  if (left.mode != right.mode || left.shield != right.shield || left.confidence != right.confidence ||
      left.estimateAvailable != right.estimateAvailable || left.stopLatched != right.stopLatched ||
      left.fault != right.fault || left.movementActive != right.movementActive ||
      left.rainEventCount != right.rainEventCount) return true;
  return !left.movementActive && fabsf(left.estimatedPositionPct - right.estimatedPositionPct) >= 0.1f;
}

void persistIfNeeded(bool force = false) {
  const Snapshot snapshot = controller.snapshot();
  if (!force && persistedSnapshotKnown && !snapshotsDiffer(snapshot, lastPersistedSnapshot)) return;
  if (!filesystemReady || !storage.saveSnapshot(snapshot)) {
    filesystemReady = false;
    controller.raiseFault(Fault::STORAGE_ERROR, millis());
    applyActuatorOutput();
    return;
  }
  lastPersistedSnapshot = snapshot;
  persistedSnapshotKnown = true;
}

bool controlAuthConfigured() { return strlen(CONTROL_TOKEN) > 0; }

bool constantTimeTokenMatch(const String& supplied) {
  const size_t expectedLength = strlen(CONTROL_TOKEN);
  const size_t suppliedLength = supplied.length();
  size_t maximum = expectedLength > suppliedLength ? expectedLength : suppliedLength;
  size_t difference = expectedLength ^ suppliedLength;
  for (size_t index = 0; index < maximum; ++index) {
    const char expected = index < expectedLength ? CONTROL_TOKEN[index] : 0;
    const char actual = index < suppliedLength ? supplied[index] : 0;
    difference |= static_cast<uint8_t>(expected ^ actual);
  }
  return difference == 0;
}

bool authorized() {
  if (!controlAuthConfigured()) return true;
  return constantTimeTokenMatch(server.header("X-GreenGuard-Token"));
}

void addSecurityHeaders(bool cacheAssets = false) {
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.sendHeader("Referrer-Policy", "no-referrer");
  server.sendHeader("Content-Security-Policy", "default-src 'self'; connect-src 'self'; img-src 'self'; style-src 'self'; script-src 'self'");
  server.sendHeader("Cache-Control", cacheAssets ? "public, max-age=300" : "no-store");
}

void sendJson(JsonDocument& document, int status = 200) {
  String body;
  serializeJson(document, body);
  addSecurityHeaders(false);
  server.send(status, "application/json; charset=utf-8", body);
}

void sendUnauthorized() {
  JsonDocument response;
  response["received"] = false;
  response["accepted"] = false;
  response["message"] = "CONTROL_TOKEN không đúng hoặc còn thiếu.";
  sendJson(response, 401);
}

bool parseJsonBody(JsonDocument& document) {
  if (!server.hasArg("plain") || server.arg("plain").length() == 0 || server.arg("plain").length() > 2048) return false;
  return deserializeJson(document, server.arg("plain")) == DeserializationError::Ok;
}

void serveFile(const char* path, const char* mime, bool cacheAsset) {
  if (!filesystemReady) {
    server.send(503, "text/plain; charset=utf-8", "LittleFS unavailable");
    return;
  }
  File file = LittleFS.open(path, "r");
  if (!file) {
    server.send(404, "text/plain; charset=utf-8", "Not found");
    return;
  }
  addSecurityHeaders(cacheAsset);
  server.streamFile(file, mime);
  file.close();
}

void statusRoute() {
  const uint32_t nowMs = millis();
  JsonDocument response;
  response["protocolVersion"] = 2;
  response["deviceName"] = hardware::DEVICE_NAME;
  response["confirmedController"] = "NodeMCU 1.0 (ESP-12E Module)";
  response["platformioBoard"] = "nodemcuv2";
  response["actuatorDryRun"] = hardware::ACTUATOR_DRY_RUN;
  response["controlAuthConfigured"] = controlAuthConfigured();
  response["filesystemReady"] = filesystemReady;
  response["uptimeMs"] = nowMs;
  response["mode"] = toString(controller.mode());
  response["rainState"] = toString(controller.rain());
  response["rainRawWet"] = sensorRawWet;
  response["rainDigitalLevel"] = sensorDigitalLevel == HIGH ? "HIGH" : "LOW";
  response["shieldState"] = toString(controller.shield());
  response["motionState"] = toString(controller.motion());
  response["positionConfidence"] = toString(controller.confidence());
  response["estimateAvailable"] = controller.estimateAvailable();
  response["estimatedPositionPct"] = controller.estimatedPositionPct();
  response["stopLatched"] = controller.stopLatched();
  response["fault"] = toString(controller.fault());
  response["rainEventCount"] = controller.rainEventCount();
  response["movementElapsedMs"] = controller.movementElapsedMs(nowMs);
  response["movementDurationMs"] = controller.movementDurationMs();
  response["lastRequestId"] = controller.lastRequestId();
  response["lastCommand"] = toString(controller.lastCommand());
  response["lastCommandResult"] = toString(controller.lastCommandResult());
  response["commandPhase"] = toString(controller.commandPhase());
  response["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  response["wifiRssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  response["ipAddress"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  response["freeHeapBytes"] = ESP.getFreeHeap();
  response["useLimitSwitches"] = hardware::USE_LIMIT_SWITCHES;
  sendJson(response);
}

void commandRoute() {
  if (!authorized()) {
    sendUnauthorized();
    return;
  }
  JsonDocument request;
  JsonDocument response;
  if (!parseJsonBody(request)) {
    response["received"] = false;
    response["accepted"] = false;
    response["message"] = "JSON không hợp lệ hoặc quá lớn.";
    sendJson(response, 400);
    return;
  }
  const bool versionValid = request["protocolVersion"].is<uint8_t>() && request["protocolVersion"].as<uint8_t>() == 2;
  const bool requestIdValid = request["requestId"].is<uint32_t>() && request["requestId"].as<uint32_t>() != 0;
  const uint32_t requestId = requestIdValid ? request["requestId"].as<uint32_t>() : 0;
  const Command commandValue = versionValid && requestIdValid ? commandFromString(request["command"] | "") : Command::INVALID;
  const CommandResponse outcome = controller.command(commandValue, requestId, readInputs(millis()));
  applyActuatorOutput();
  persistIfNeeded(outcome.accepted);
  response["received"] = versionValid && requestIdValid;
  response["accepted"] = outcome.accepted;
  response["duplicate"] = outcome.result == CommandResult::DUPLICATE;
  response["requestId"] = requestId;
  response["command"] = toString(commandValue);
  response["result"] = toString(outcome.result);
  response["phase"] = toString(outcome.phase);
  const int status = outcome.result == CommandResult::INVALID ? 400 :
      (outcome.accepted || outcome.result == CommandResult::DUPLICATE ? 200 : 409);
  sendJson(response, status);
}

void configGetRoute() {
  JsonDocument response;
  response["wetConfirmMs"] = deviceConfig.controller.wetConfirmMs;
  response["dryConfirmMs"] = deviceConfig.controller.dryConfirmMs;
  response["directionDeadTimeMs"] = deviceConfig.controller.directionDeadTimeMs;
  response["inferredFullTravelMs"] = deviceConfig.controller.inferredFullTravelMs;
  response["motorMaxRuntimeMs"] = deviceConfig.controller.motorMaxRuntimeMs;
  response["rainActiveLow"] = deviceConfig.rainActiveLow;
  response["deployUsesRpwm"] = deviceConfig.deployUsesRpwm;
  response["motorPwm"] = deviceConfig.motorPwm;
  response["useLimitSwitches"] = hardware::USE_LIMIT_SWITCHES;
  sendJson(response);
}

void configPostRoute() {
  if (!authorized()) {
    sendUnauthorized();
    return;
  }
  JsonDocument request;
  JsonDocument response;
  if (!parseJsonBody(request)) {
    response["success"] = false;
    response["message"] = "JSON không hợp lệ hoặc quá lớn.";
    sendJson(response, 400);
    return;
  }
  DeviceConfig candidate = deviceConfig;
  if (!request["wetConfirmMs"].isNull()) candidate.controller.wetConfirmMs = request["wetConfirmMs"].as<uint32_t>();
  if (!request["dryConfirmMs"].isNull()) candidate.controller.dryConfirmMs = request["dryConfirmMs"].as<uint32_t>();
  if (!request["directionDeadTimeMs"].isNull()) candidate.controller.directionDeadTimeMs = request["directionDeadTimeMs"].as<uint32_t>();
  if (!request["inferredFullTravelMs"].isNull()) candidate.controller.inferredFullTravelMs = request["inferredFullTravelMs"].as<uint32_t>();
  if (!request["motorMaxRuntimeMs"].isNull()) candidate.controller.motorMaxRuntimeMs = request["motorMaxRuntimeMs"].as<uint32_t>();
  if (!request["rainActiveLow"].isNull()) candidate.rainActiveLow = request["rainActiveLow"].as<bool>();
  if (!request["deployUsesRpwm"].isNull()) candidate.deployUsesRpwm = request["deployUsesRpwm"].as<bool>();
  if (!request["motorPwm"].isNull()) candidate.motorPwm = request["motorPwm"].as<uint16_t>();
  candidate.controller.useLimitSwitches = hardware::USE_LIMIT_SWITCHES;

  if (!validDeviceConfig(candidate) || controller.motion() != MotionState::STOPPED) {
    response["success"] = false;
    response["message"] = "Cấu hình không hợp lệ hoặc motor chưa dừng.";
    sendJson(response, 409);
    return;
  }
  if (!filesystemReady || !storage.saveConfig(candidate) || !controller.configure(candidate.controller)) {
    controller.raiseFault(Fault::STORAGE_ERROR, millis());
    applyActuatorOutput();
    response["success"] = false;
    response["message"] = "Không thể lưu cấu hình; GreenGuard đã dừng ở trạng thái lỗi.";
    sendJson(response, 500);
    return;
  }
  deviceConfig = candidate;
  response["success"] = true;
  response["message"] = "Đã lưu cấu hình.";
  sendJson(response);
}

void setupRoutes() {
  server.collectHeaders("X-GreenGuard-Token");
  server.on("/", HTTP_GET, [] { serveFile("/index.html", "text/html; charset=utf-8", false); });
  server.on("/style.css", HTTP_GET, [] { serveFile("/style.css", "text/css; charset=utf-8", true); });
  server.on("/protocol.js", HTTP_GET, [] { serveFile("/protocol.js", "application/javascript; charset=utf-8", true); });
  server.on("/app.js", HTTP_GET, [] { serveFile("/app.js", "application/javascript; charset=utf-8", true); });
  server.on("/api/status", HTTP_GET, statusRoute);
  server.on("/api/command", HTTP_POST, commandRoute);
  server.on("/api/config", HTTP_GET, configGetRoute);
  server.on("/api/config", HTTP_POST, configPostRoute);
  server.on("/health", HTTP_GET, [] { server.send(200, "text/plain; charset=utf-8", "OK"); });
  server.onNotFound([] {
    JsonDocument response;
    response["success"] = false;
    response["message"] = "Không tìm thấy đường dẫn.";
    sendJson(response, 404);
  });
  server.begin();
}

bool wifiConfigured() { return strlen(WIFI_SSID) > 0; }

void updateWiFi(uint32_t nowMs) {
  static bool wasConnected = false;
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !wasConnected) {
    Serial.print(F("Wi-Fi connected: "));
    Serial.println(WiFi.localIP());
    mdnsStarted = MDNS.begin(hardware::MDNS_NAME);
    if (mdnsStarted) Serial.println(F("Dashboard: http://greenguard.local"));
  } else if (!connected && wasConnected) {
    Serial.println(F("Wi-Fi lost; local rain control continues."));
    if (mdnsStarted) MDNS.close();
    mdnsStarted = false;
  }
  wasConnected = connected;
  if (!connected && wifiConfigured() && intervalElapsed(nowMs, lastWifiAttemptMs, hardware::WIFI_RETRY_INTERVAL_MS)) {
    lastWifiAttemptMs = nowMs;
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
  if (mdnsStarted) MDNS.update();
}

void setup() {
  initializeActuatorPins();
  pinMode(hardware::RAIN_DO_PIN, INPUT);
  if (hardware::USE_LIMIT_SWITCHES) {
    pinMode(hardware::RETRACTED_LIMIT_PIN, INPUT);
    pinMode(hardware::DEPLOYED_LIMIT_PIN, INPUT);
  }

  Serial.begin(115200);
  Serial.println();
  Serial.println(F("GreenGuard boot: NodeMCU 1.0 ESP-12E / nodemcuv2"));
  Serial.printf("ACTUATOR_DRY_RUN=%s\n", hardware::ACTUATOR_DRY_RUN ? "true" : "false");

  filesystemReady = storage.begin();
  Snapshot restored;
  LoadResult configResult = LoadResult::MISSING;
  LoadResult snapshotResult = LoadResult::MISSING;
  if (filesystemReady) {
    configResult = storage.loadConfig(deviceConfig);
    snapshotResult = storage.loadSnapshot(restored);
    if (configResult == LoadResult::MISSING && !storage.saveConfig(deviceConfig)) filesystemReady = false;
  }
  deviceConfig.controller.useLimitSwitches = hardware::USE_LIMIT_SWITCHES;
  controller.configure(deviceConfig.controller);
  sensorDigitalLevel = digitalRead(hardware::RAIN_DO_PIN);
  sensorRawWet = deviceConfig.rainActiveLow ? sensorDigitalLevel == LOW : sensorDigitalLevel == HIGH;
  const Inputs initialInputs = readInputs(millis());
  const bool persistenceValid = configResult != LoadResult::INVALID && snapshotResult != LoadResult::INVALID;
  controller.begin(restored, initialInputs, persistenceValid);
  if (!filesystemReady) controller.raiseFault(Fault::STORAGE_ERROR, millis());
  applyActuatorOutput();
  if (filesystemReady) persistIfNeeded(true);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hardware::MDNS_NAME);
  if (wifiConfigured()) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWifiAttemptMs = millis();
  } else {
    Serial.println(F("Wi-Fi credentials are empty; automation remains offline."));
  }
  if (!controlAuthConfigured()) Serial.println(F("WARNING: local command token is not configured."));
  setupRoutes();
}

void loop() {
  const uint32_t nowMs = millis();
  const Inputs inputs = readInputs(nowMs);
  controller.tick(inputs);
  applyActuatorOutput();
  persistIfNeeded();
  server.handleClient();
  updateWiFi(nowMs);
  if (intervalElapsed(nowMs, lastDiagnosticMs, hardware::DIAGNOSTIC_INTERVAL_MS)) {
    lastDiagnosticMs = nowMs;
    Serial.printf("mode=%s rain=%s shield=%s motion=%s fault=%s heap=%u\n",
        toString(controller.mode()), toString(controller.rain()), toString(controller.shield()),
        toString(controller.motion()), toString(controller.fault()), ESP.getFreeHeap());
  }
  yield();
}
