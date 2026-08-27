# System design

## Purpose and definitions

GreenGuard protects plants from sustained rain with a motorized shield controlled locally by a NodeMCU 1.0 ESP-12E. The safety controller runs without the browser, Wi-Fi, or Internet.

- `DEPLOYED`: the shield covers and protects the plant.
- `RETRACTED`: the shield is moved away and the plant is uncovered.
- `ESTIMATED`: position came from elapsed motor time.
- `USER_CALIBRATED`: a person visually confirmed and marked an endpoint.
- `LIMIT_CONFIRMED`: an enabled physical endpoint switch is active.

The code and UI do not use “open/closed” as motor commands because those words are ambiguous for this mechanism.

## Requirements matrix

| Requirement | Implemented behavior | Observable status |
| --- | --- | --- |
| Sustained wet in AUTO | Wet must remain stable for 3 s by default, then deploy | Rain `WET`; motion passes through dead-time to `DEPLOYING` |
| Brief rain noise | Candidate timer resets on every opposite edge | Stable rain does not change; no motion |
| Dry confirmation | Dry must remain stable for 120 s by default | Rain remains `WET` until the full interval passes |
| Rain returns during dry confirmation | Dry candidate is cancelled immediately | No retraction |
| Unknown position | Confirmed wet may run a full deploy cycle; dry never triggers a blind retract | `UNKNOWN` or time estimate; protective direction only |
| Offline operation | Sensor, FSM, outputs, persistence, timeout, and fault handling have no network dependency | Wi-Fi status may be offline while AUTO continues |
| STOP | Motor outputs become OFF immediately; mode becomes MANUAL; STOP latch persists | Phase `STOPPED`, motion `STOPPED` |
| Reverse direction | Both PWM paths turn off and remain off through configurable dead-time (300 ms default) | Motion `REVERSAL_DEAD_TIME`, drive `OFF` |
| Runtime bound | Every physical movement has an absolute maximum (35 s default) | `MOTOR_TIMEOUT`, outputs OFF, fault lockout |
| Interrupted reboot | A persisted active movement invalidates position | `UNKNOWN`, STOP latch set |
| Corrupt persistence | Invalid enums/schema/ranges fail closed | `PERSISTENCE_INVALID` fault |
| Limit-input feature | Disabled by default; when enabled, either endpoint confirms position and both active produce a fault | `LIMIT_CONFIRMED` or `LIMIT_CONFLICT` |
| Request correlation | Every local command requires a nonzero uint32 request ID | Status reports the exact ID, result, and phase |

## State model

Three independent axes prevent misleading UI state:

| Axis | States |
| --- | --- |
| Operating mode | `AUTO`, `MANUAL` (a non-`NONE` fault is shown separately and blocks motion) |
| Shield | `UNKNOWN`, `RETRACTED`, `DEPLOYED`, `STOPPED_PARTIAL` |
| Motion | `STOPPED`, `DEPLOYING`, `RETRACTING`, `REVERSAL_DEAD_TIME` |

Faults are `NONE`, `MOTOR_TIMEOUT`, `LIMIT_CONFLICT`, `PERSISTENCE_INVALID`, or `STORAGE_ERROR`. A fault always turns drive output OFF and latches STOP. `RESET_FAULT` clears it only into stopped MANUAL mode; it does not restart motion.

## Transition rules

1. Startup configures RPWM and LPWM as outputs and writes both LOW before storage, Wi-Fi, web, or sensor logic.
2. The rain input starts `UNKNOWN`. It becomes `WET` or `DRY` only after its own confirmation interval.
3. AUTO + WET requests `DEPLOYED`, even from unknown position because this is the protective direction.
4. AUTO + DRY requests `RETRACTED` only when a position estimate or confirmation exists.
5. Selecting MANUAL stops current motion. `DEPLOY` and `RETRACT` are then explicit; retraction is rejected unless rain is confirmed `DRY`.
6. If confirmed rain returns during manual retraction, GreenGuard stops, observes dead-time, and deploys. It remains in MANUAL afterward.
7. Every new direction begins with both PWM outputs off. The state machine cannot represent two active directions, and the output mapper independently guarantees only one PWM duty can be nonzero.
8. STOP overrides mode/motion. A new explicit AUTO or accepted manual motion can clear the STOP latch; fault reset alone keeps it latched.
9. With the limit-input feature disabled, timed completion sets endpoint position to `ESTIMATED` even when switch wires are physically present. It never claims physical confirmation.

All time comparisons cast `now - since` to `uint32_t`, so behavior remains correct across `millis()` wraparound.

## Network architecture

```text
rain DO ──> non-blocking filter ──> controller FSM ──> dry-run/output interlock ──> RPWM/LPWM
                                      │
                                      ├──> LittleFS state/config
                                      └──> local REST status

browser on same LAN ──> ESP8266WebServer ──> request ID + token check ──> controller command
```

The ESP8266 serves the dashboard from LittleFS at `http://greenguard.local` or its DHCP address. GreenGuard has no ThingSpeak, Firebase, Blynk, MQTT, Vercel, or Internet dependency.

## Local security boundary

GET status/config is visible to the local network. State-changing POST requests use `X-GreenGuard-Token` when `CONTROL_TOKEN` is nonempty. The dashboard stores that token only in `sessionStorage`, not in a URL or repository file. An empty token deliberately leaves LAN control unauthenticated and produces a warning. This is not TLS and must not be exposed directly to the Internet.

## Current limitations

- Binary rain DO cannot diagnose every stuck-wet/stuck-dry sensor failure.
- Timed travel cannot detect jams, slipping, or real endpoints.
- One remembered request ID provides idempotence for immediate local retries, not an audit log.
- Local token authentication without HTTPS does not protect against a hostile LAN observer.
- The as-built GPIO connections are recorded, but limit-switch behavior, exact direction-to-terminal polarity, full-travel time, stall behavior, complete power topology, and shared-ground measurements still need further characterization.
