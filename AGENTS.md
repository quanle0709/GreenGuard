# Repository guide

- Confirmed target: NodeMCU 1.0 (ESP-12E Module), ESP8266; `board = nodemcuv2`.
- Keep `hardware::ACTUATOR_DRY_RUN = true` in committed code.
- Exact GPIO wiring and all motor/sensor/power behavior remain unverified until documented measurements exist.
- Do not introduce ESP32 APIs, targets, or migration code unless the owner explicitly changes the controller.
- Build: `pio run -e nodemcuv2`; filesystem: `pio run -e nodemcuv2 -t buildfs`.
- Native tests: prepend a host C++ compiler to PATH, then `pio test -e native`.
- Web checks: `npm test` and `npm run check`; no npm packages are required.
- `src/` holds ESP8266 integration, `lib/GreenGuardCore/` holds portable safety logic, `data/` holds LittleFS UI, and `test/` holds native/web tests.
- Never commit `include/secrets.h`, `.pio`, credentials, generated binaries, test caches, or local absolute paths.
- Update `docs/TEST_RESULTS.md` only from commands actually run; never claim physical validation from simulation.
