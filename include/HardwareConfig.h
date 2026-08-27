#pragma once

#include <Arduino.h>

namespace hardware {

// Confirmed target and as-built mapping: NodeMCU 1.0 (ESP-12E Module), ESP8266.
constexpr uint8_t RAIN_DO_PIN = D1;       // GPIO5; RainDrop DO, active-LOW in our test.
constexpr uint8_t BTS_RPWM_PIN = D5;      // GPIO14; BTS7960 RPWM.
constexpr uint8_t BTS_LPWM_PIN = D6;      // GPIO12; BTS7960 LPWM.
constexpr uint8_t BTS_ENABLE_PIN = D2;    // GPIO4; connected to BTS7960 R_EN and L_EN control.
constexpr uint8_t RETRACTED_LIMIT_PIN = D7;  // GPIO13; retracted limit-switch connection.
constexpr uint8_t DEPLOYED_LIMIT_PIN = D0;   // GPIO16; deployed limit-switch connection.

// Public firmware settings; physical switch contact polarity and endpoint accuracy are not measured.
constexpr bool LIMIT_ACTIVE_LOW = true;
constexpr bool USE_LIMIT_SWITCHES = false;

// The as-built D2 connection is confirmed, but public firmware control remains disabled.
// Before enabling it, measure the node and ensure no external 5 V source shares the GPIO.
constexpr bool CONTROL_BTS_ENABLE = false;

// Mandatory safe default: firmware logic runs, but RPWM and LPWM remain LOW.
constexpr bool ACTUATOR_DRY_RUN = true;

constexpr uint32_t SENSOR_SAMPLE_INTERVAL_MS = 50;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 15000;
constexpr uint32_t DIAGNOSTIC_INTERVAL_MS = 10000;
constexpr char DEVICE_NAME[] = "GreenGuard";
constexpr char MDNS_NAME[] = "greenguard";

}  // namespace hardware
