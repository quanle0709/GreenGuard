# GreenGuard test results

## Automated repository verification

Automated checks validate the committed ESP8266 build, filesystem image, controller state machine, dashboard protocol, repository structure, and safe defaults. They are separate from the physical prototype validation recorded below.

| Check | Command | Result |
| --- | --- | --- |
| Clean ESP8266 firmware | `pio run -e nodemcuv2 -t clean`, then `pio run -e nodemcuv2` | PASS; PlatformIO identified `board: nodemcuv2`, NodeMCU 1.0 ESP-12E, ESP8266 80 MHz / 80 KiB RAM / 4 MiB flash |
| Firmware size | Same clean build | RAM 31,720 / 81,920 bytes (38.7%); flash 382,923 / 1,044,464 bytes (36.7%) |
| LittleFS image | `pio run -e nodemcuv2 -t buildfs` | PASS; image contains `/app.js`, `/index.html`, `/protocol.js`, and `/style.css` |
| Portable controller | `pio test -e native` | PASS; 45/45 scenarios, 132 runtime assertions |
| JavaScript syntax | `node --check` for dashboard, protocol, mock server, and tests | PASS |
| Web, protocol, authentication, and integration | `npm test` | PASS; 11/11 tests, 52 assertions |
| Project structure and safety defaults | `npm run check` | PASS; required artifacts, JavaScript syntax, `nodemcuv2`, dry-run default, ignore rules, links, and controller scan |
| PlatformIO configuration | `pio project config` | PASS; computed `default_envs = nodemcuv2`, `board = nodemcuv2`, filesystem `littlefs` |
| Secret-like text scan | `git grep` plus ignored-file review | PASS; only expected macro names, includes, and examples; no credential values committed |
| Generated artifact and archive scan | `git ls-files` pattern scan | PASS; no `.pio`, `node_modules`, object, binary, or ZIP build artifact tracked |
| Local path scan | Repository text search | PASS; no local absolute path in tracked repository files |
| Git whitespace check | `git diff --check` | PASS |

Current combined automated count: **56 test cases/scenarios and 184 assertions**. Scenario count is reported separately from assertions.

### Automated scenario coverage

Native controller tests cover startup dry, startup wet, known/unknown persistence, corrupt persistence, reboot during motion, sustained rain, wet noise, rapid wet/dry noise, cancelled dry confirmation, sustained dry, manual deploy/retract, rain-blocked retract, STOP in both directions, repeated commands, duplicate IDs, mode conflicts, reversal dead-time, mutually exclusive PWM, dry-run output, enable behavior, maximum runtime, fault lockout/reset, AUTO↔MANUAL, malformed or zero-ID commands, `millis()` wraparound, limits disabled, each limit active, both limits active, rain reversal during manual retract, persisted STOP, calibration, partial estimates, snapshot movement marking, offline autonomy, and frequent-loop dead-time regression.

Web tests cover nonzero request IDs, delayed and mismatched acknowledgements, separate terminal phases, malformed status, stale/disconnect logic, exact Vietnamese controls, token and request transport, honest timeout copy, mobile/reduced-motion/focus styling, mock status and assets, unauthorized and malformed requests, login-token → command → started → completed flow, duplicate requests, and request-specific wet rejection.

## Physical prototype validation

We completed the physical GreenGuard prototype, applied the updated working firmware, and performed the recorded validation on **June 26, 2026**. This physical test is distinct from automated repository checks and from the later GitHub publication date.

### Confirmed as-built connections

| NodeMCU pin | GPIO | Physical connection |
| --- | ---: | --- |
| D1 | GPIO5 | RainDrop DO |
| D5 | GPIO14 | BTS7960 RPWM |
| D6 | GPIO12 | BTS7960 LPWM |
| D2 | GPIO4 | BTS7960 R_EN and L_EN control |
| D7 | GPIO13 | Retracted limit-switch connection |
| D0 | GPIO16 | Deployed limit-switch connection |

### Recorded physical results

| Measurement | Result |
| --- | ---: |
| Continuous deploy–retract testing | 10 cycles with the real mechanical load |
| Motor supply before / during movement | 12.18 V / 11.72 V |
| Supply voltage drop | 3.8% |
| Lowest 3V3 rail during motor startup | 3.17 V |
| NodeMCU resets | None |
| RainDrop DO dry / wet | 3.27 V / 0.08 V, active-LOW |
| Loaded running motor current | 2.63 A |
| Maximum measured startup current | 7.20 A |
| Direction-reversal dead-time | 307 ms |
| Highest motor / wire-and-connector temperature | 51°C / 34°C after 10 cycles |
| Evaluation | **PASS with conditions within this test scope** |

The observed 3V3 rail remained stable enough for the measured startup events, and the 307 ms dead-time closely matched the configured 300 ms target. RainDrop DO remained within the ESP8266 GPIO range in our tested arrangement.

The result does not establish long-term endurance, outdoor weather resistance, ingress protection, corrosion resistance, every obstruction condition, stall behavior, final fuse selection, or lifetime reliability. Limit-switch wires are physically present, but switch polarity, calibration, endpoint accuracy, and full-travel time were not included in the recorded measurement set.

Our prototype used an actuator-enabled local configuration. The committed repository intentionally retains `ACTUATOR_DRY_RUN=true`, `CONTROL_BTS_ENABLE=false`, and `USE_LIMIT_SWITCHES=false`. See the [hardware validation and reproduction checklist](HARDWARE_TEST_CHECKLIST.md) for the remaining safety work.
