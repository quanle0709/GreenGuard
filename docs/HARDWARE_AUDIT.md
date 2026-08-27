# Hardware audit

Audit date: 2026-08-26; physical-validation update recorded 2026-08-27. This audit separates confirmed components and owner-supplied measurements from exact wiring details and tests that remain undocumented.

## Evidence table

| Component | Confirmed model / result | Supply or logic observation | Candidate pin | Evidence | Remaining uncertainty |
| --- | --- | --- | --- | --- | --- |
| Controller | **NodeMCU 1.0 (ESP-12E Module), ESP8266** | Lowest observed 3V3 rail at motor startup: 3.17 V; no reset in 10 cycles | PlatformIO `nodemcuv2` | Owner correction and physical test | Exact controller power wiring is not recorded as an as-built diagram |
| Motor driver | **BTS7960 / HW-039** | Drove the loaded motor through 10 deploy–retract cycles | D5/GPIO14 RPWM and D6/GPIO12 LPWM in firmware | Owner physical-validation update | Actual terminal-to-GPIO table, cooling margin, and behavior outside the test scope remain undocumented |
| Driver enable | R_EN and L_EN present on the driver | Wiring voltage/path not supplied | External HIGH is a repository lead; optional D2/GPIO4 remains disabled | Repository history plus confirmed HW-039 use | Exact EN wiring remains unverified; never join an existing 5 V node to ESP8266 GPIO |
| Rain module | **RainDrop digital sensor using active-LOW DO** | Dry 3.27 V; wet 0.08 V | D1/GPIO5 in firmware | Owner measurements | Exact VCC and physical pin-to-wire record were not supplied; long-term wet/outdoor behavior remains untested |
| Actuator | **12 V DC geared motor** | 2.63 A normal loaded motion; 7.20 A maximum measured startup current | BTS7960 motor outputs | Owner measurements and 10 loaded cycles | 7.20 A is not a stall-current test; full-travel time, jam response, and lifetime endurance remain undocumented |
| Retracted limit | Not confirmed | 3.3 V active-low candidate only | Optional D7/GPIO13, disabled | Firmware option only | Presence, contact type, placement, and polarity are unknown |
| Deployed limit | Not confirmed | 3.3 V active-low candidate only | Optional D0/GPIO16 with external pull-up, disabled | Firmware option only | Presence, contact type, placement, and polarity are unknown |
| Power system | **12 V, 10 A supply and XL4005 step-down module** | Motor rail 12.18 V before and 11.72 V during motion; 3.8% drop | n/a | Owner component and measurement update | Exact topology, shared-ground record, fuse, wire gauge, connector ratings, ripple, and weather protection remain undocumented |

Confirmed hardware and measurements above come from the owner's completed prototype test. Candidate pins and undocumented wiring paths are not promoted to physical facts.

## Physical validation summary

Firmware based on commit `441f91e` was flashed to the real prototype on 2026-08-26. It completed 10 consecutive deploy–retract cycles with the mechanical load. Measured reversal dead-time was 307 ms versus the configured 300 ms. Peak temperatures after the cycles were 51°C at the motor and 34°C at the wires/connectors. The result is **PASS with conditions within this test scope**.

This confirms observed function and the listed electrical values for those 10 cycles. It does not establish long-term endurance, outdoor weather resistance, ingress protection, every obstruction condition, stall behavior, or lifetime reliability. The actuator-enabled test configuration was local; public source remains `ACTUATOR_DRY_RUN=true`.

## Controller and pin audit

`platformio.ini` uses `board = nodemcuv2`. PlatformIO identifies that board as NodeMCU 1.0 (ESP-12E Module), ESP8266, 80 MHz, 80 KiB RAM, and 4 MiB flash. The firmware uses ESP8266 Arduino APIs (`ESP8266WiFi`, `ESP8266WebServer`, mDNS, LittleFS) and was compiled against that board profile.

The NodeMCU labels map as follows: D0→GPIO16, D1→GPIO5, D2→GPIO4, D3→GPIO0, D4→GPIO2, D5→GPIO14, D6→GPIO12, D7→GPIO13, and D8→GPIO15. D3/GPIO0, D4/GPIO2, and D8/GPIO15 participate in boot selection; GreenGuard deliberately assigns no default sensor, PWM, enable, or limit function to those three pins. D1, D2, D5, D6, and D7 avoid the boot straps. D0/GPIO16 is only an optional disabled limit input and must use a verified external pull-up rather than relying on a normal internal pull-up.

Sources:

- [PlatformIO NodeMCU 1.0 board definition](https://docs.platformio.org/en/latest/boards/espressif8266/nodemcuv2.html)
- [ESP8266 Arduino core pin mapping and filesystem documentation](https://arduino-esp8266.readthedocs.io/en/stable/)
- [Espressif boot-mode selection](https://docs.espressif.com/projects/esptool/en/latest/esp8266/advanced-topics/boot-mode-selection.html)
- [Espressif ESP8266EX datasheet](https://www.espressif.com/sites/default/files/documentation/0a-esp8266ex_datasheet_en.pdf)

## Electrical safety findings

- Treat every ESP8266 GPIO as 3.3 V-only. Do not connect an unmeasured 5 V DO, R_EN, L_EN, or driver output to it.
- RainDrop DO was measured at 3.27 V dry and 0.08 V wet, confirming active-LOW behavior within the ESP8266 GPIO range during this test. Preserve this voltage boundary if wiring or sensor power changes.
- The HW-039/BTS7960 module identity is confirmed, and the prototype operated with it. The BTS7960 silicon datasheet lists logic thresholds compatible with a 3.3 V HIGH, but the exact module terminal wiring still needs an as-built record. See the [Infineon BTS7960 datasheet](https://www.infineon.com/assets/row/public/documents/10/57/infineon-bts7960-ds-en.pdf).
- If R_EN/L_EN are really tied high, firmware can stop only by setting both PWM inputs LOW. If they are instead moved to D2, remove every 5 V enable connection first and set `CONTROL_BTS_ENABLE=true` only after checking the new wiring. Never combine the two arrangements.
- The confirmed power components are a 12 V, 10 A supply and XL4005 step-down module. The observed motor rail dropped from 12.18 V to 11.72 V (3.8%), while the lowest observed 3V3 rail was 3.17 V without a NodeMCU reset. Exact topology and shared-ground wiring still need documentation.
- Supply and fuse sizing must be based on a proper circuit review and stall-current evidence, not the 7.20 A startup maximum or the headline current printed on a driver. Add or verify a physical emergency disconnect, appropriately rated fuse, wire/connector sizing, strain relief, suppression, and a weatherproof enclosure.
- No current sensor is evidenced. Software cannot detect a jam or overload merely from elapsed time.

## Position truth

With `USE_LIMIT_SWITCHES=false`, reaching 0% or 100% means only that the configured time elapsed. The dashboard labels this `ESTIMATED`; it is not endpoint evidence. Voltage, load, friction, slipping, obstruction, and interrupted movement cause drift. STOP produces `STOPPED_PARTIAL`. A reboot after a saved active movement produces `UNKNOWN`. A manually observed endpoint is `USER_CALIBRATED`, while an enabled physical switch would be `LIMIT_CONFIRMED`.

The prototype physically completed deploy–retract cycles, but the supplied validation record does not identify which GPIO/terminal produces each direction, whether endpoint switches exist, the endpoint mechanism, or the actual full-travel time. `deployUsesRpwm` therefore remains a configurable mapping rather than a documented wiring fact.

## Scope tiers

Implemented and observed with the confirmed prototype hardware: local wet/dry filtering, AUTO/MANUAL, deploy/retract/STOP, reversal dead-time, maximum runtime, offline autonomy, local dashboard, and explicitly estimated position. The committed build remains in dry-run by default.

Small hardware additions: two endpoint switches, current/stall sensing, correctly sized fuse, physical emergency disconnect, verified level shifting, improved regulated power, and weatherproofing.

Longer term: soil moisture, forecast-aware policy, battery/solar monitoring, notifications, history, and multiple zones. These are not part of this rebuild.
