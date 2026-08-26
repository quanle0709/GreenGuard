# Test results

Run date: 2026-08-26, Asia/Saigon. Tests ran from the canonical D-drive repository. No result from the previous rebuild was trusted or copied as a pass.

## Automated checks completed so far

| Check | Command | Result |
| --- | --- | --- |
| Clean ESP8266 firmware | `pio run -e nodemcuv2 -t clean`, then `pio run -e nodemcuv2` | PASS. PlatformIO printed `board: nodemcuv2`, `NodeMCU 1.0 (ESP-12E Module)`, ESP8266 80 MHz / 80 KiB RAM / 4 MiB flash |
| Firmware size | same clean build | RAM 31,720 / 81,920 bytes (38.7%); flash 382,923 / 1,044,464 bytes (36.7%) |
| LittleFS image | `pio run -e nodemcuv2 -t buildfs` | PASS; image contains `/app.js`, `/index.html`, `/protocol.js`, `/style.css` |
| Portable controller | `pio test -e native` with MinGW `g++` on PATH | PASS; 45/45 scenarios, 132 runtime assertions |
| JavaScript syntax | `node --check` for dashboard, protocol, mock server, and tests | PASS |
| Web/protocol/auth/integration | `node --test` | PASS; 11/11 tests, 52 assertions |
| Project structure/syntax | `node scripts/check-project.mjs` | PASS; 17 required artifacts, JavaScript syntax, board target, dry-run, ignore rules, and stale-controller source scan |
| PlatformIO configuration | `pio project config` | PASS; computed `default_envs = nodemcuv2`, `board = nodemcuv2`, filesystem `littlefs` |
| Secret-like text scan | `git grep` plus ignored-file review | PASS; only expected macro names/includes/examples, no credential values committed |
| Generated artifact/archive scan | `git ls-files` pattern scan | PASS; no `.pio`, `node_modules`, binary, object, or ZIP artifact tracked |
| Local path scan | `rg` for Windows absolute paths outside ignored output | PASS; no local absolute path in repository files |
| Git whitespace check | `git diff --check` | PASS; no whitespace errors |
| Serial-device detection | `pio device list` plus Windows serial/PnP inventory | No NodeMCU/USB UART candidate. COM3/4/6/7 are Bluetooth serial links; no flash attempted |
| Visual browser rendering | in-app browser skill against `http://127.0.0.1:4173` | BLOCKED; runtime returned no available browser instances after the required recovery check |

Current combined count: **56 test cases/scenarios and 184 assertions**. Scenario count is reported separately from assertions to avoid inflating coverage.

## Scenario coverage

Native controller tests cover startup dry, startup wet, known/unknown persistence, corrupt persistence, reboot during motion, sustained rain, short wet noise, rapid wet/dry noise, cancelled dry confirmation, sustained dry, manual deploy/retract, rain-blocked retract, STOP in both directions, repeated commands, duplicate IDs, mode conflicts, reversal dead-time, mutually exclusive PWM, dry-run output, optional enable behavior, maximum runtime, fault lockout/reset, AUTO↔MANUAL, malformed/zero-ID commands, `millis()` wraparound for sensor and motion, limits disabled, each limit active, both limits active, rain reversal during manual retract, persisted STOP, calibration, partial estimate, snapshot movement marking, offline autonomy, and the frequent-loop dead-time regression.

Web tests cover nonzero request IDs, delayed and mismatched acknowledgements, separate terminal phases, malformed status, stale/disconnect logic, exact Vietnamese controls, no vague ON/OFF mode switch, token/request transport, honest timeout copy, mobile/reduced-motion/focus CSS contracts, mock status/assets, unauthorized and malformed requests, login-token → command → started → completed flow, duplicates, and request-specific wet rejection.

## Environment notes

- PlatformIO Core and all packages were stored under the task workspace on D.
- The first native attempt could not run because no host `gcc/g++` was installed. A PlatformIO portable MinGW toolchain was installed on D; the unchanged suite then passed.
- The first firmware compile found an ESP8266WebServer `collectHeaders` API mismatch; it was repaired and the build passed.
- Review found and repaired a same-target AUTO race that could restart reversal dead-time on every loop tick. A dedicated regression test now passes.

## Review/repair notes

- Full diff and output-write paths were reviewed after the first pass. No blocking `delay()` remains; both PWM paths are switched off before every output change.
- The stale committed ZIP contained the contradictory ESP32/servo/ThingSpeak system. It was removed after preservation in Git history and the verified migration archive.
- ESP32 text now remains only in negative regression tests, the audit explanation, and the repository rule that forbids migrating this project to ESP32.
- Git commit/push and remote verification are delivery steps performed after this test record is finalized; their SHAs are reported in the final handoff.

## Physical verification

None. No serial device has yet been identified as GreenGuard, no firmware was flashed, and no motor/sensor/power wiring was energized or measured. The confirmed controller model comes from the owner's authoritative correction. All remaining physical items are listed in [HARDWARE_TEST_CHECKLIST.md](HARDWARE_TEST_CHECKLIST.md).
