#pragma once

#include <Arduino.h>

namespace hardware {

// Confirmed target: NodeMCU 1.0 (ESP-12E Module), ESP8266, PlatformIO nodemcuv2.
// Wiring is NOT physically confirmed. These defaults only follow repository evidence.
constexpr uint8_t RAIN_DO_PIN = D1;       // GPIO5; verify voltage and polarity.
constexpr uint8_t BTS_RPWM_PIN = D5;      // GPIO14; verify actual IBT-2 terminal.
constexpr uint8_t BTS_LPWM_PIN = D6;      // GPIO12; verify actual IBT-2 terminal.
constexpr uint8_t BTS_ENABLE_PIN = D2;    // GPIO4; unused unless CONTROL_BTS_ENABLE is true.
constexpr uint8_t RETRACTED_LIMIT_PIN = D7;  // GPIO13; optional external 10 kOhm pull-up.
constexpr uint8_t DEPLOYED_LIMIT_PIN = D0;   // GPIO16; optional external 10 kOhm pull-up.

constexpr bool LIMIT_ACTIVE_LOW = true;
constexpr bool USE_LIMIT_SWITCHES = false;

// Repository wiring claims R_EN/L_EN are externally high. This is unverified.
// Never set true until both EN wires are physically moved to D2 and are no longer tied to 5 V.
constexpr bool CONTROL_BTS_ENABLE = false;

// Mandatory safe default: firmware logic runs, but RPWM/LPWM/EN remain LOW.
constexpr bool ACTUATOR_DRY_RUN = true;

constexpr uint32_t SENSOR_SAMPLE_INTERVAL_MS = 50;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 15000;
constexpr uint32_t DIAGNOSTIC_INTERVAL_MS = 10000;
constexpr char DEVICE_NAME[] = "GreenGuard";
constexpr char MDNS_NAME[] = "greenguard";

}  // namespace hardware
