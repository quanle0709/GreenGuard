#pragma once

// Copy this file to include/secrets.h and keep secrets.h out of Git.
// Empty Wi-Fi values keep the controller offline while local automation continues.
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// Optional local command token. When empty, LAN control is deliberately unauthenticated.
// Use a long random value before putting GreenGuard on a shared network.
#define CONTROL_TOKEN ""

