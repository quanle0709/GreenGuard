#pragma once
#include <Arduino.h>

constexpr uint8_t PIN_MOTOR_RPWM = D5; // GPIO14
constexpr uint8_t PIN_MOTOR_LPWM = D6; // GPIO12
constexpr uint8_t PIN_RAIN_DIGITAL = D1; // GPIO5
constexpr uint32_t DEFAULT_FULL_TRAVEL_TIME_MS = 30000;
constexpr uint32_t DEFAULT_MOTOR_TIMEOUT_MARGIN_MS = 5000;
constexpr uint32_t MOTOR_REVERSE_DELAY_MS = 500;
constexpr uint32_t SENSOR_INTERVAL_MS = 100;
constexpr uint32_t WIFI_CONNECT_WINDOW_MS = 12000;
constexpr uint32_t WIFI_RETRY_MS = 30000;
constexpr uint32_t THINGSPEAK_INTERVAL_MS = 20000;
constexpr uint32_t DIAGNOSTIC_INTERVAL_MS = 10000;
constexpr bool DEMO_MODE = false;
constexpr char DEVICE_NAME[] = "GreenGuard";
constexpr char MDNS_NAME[] = "greenguard";
