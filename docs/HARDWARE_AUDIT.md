# Hardware audit

Audit date: 2026-08-26. This audit separates the owner's authoritative controller correction from repository claims and measurements that have not happened.

## Evidence table

| Component | Claimed model | Confirmed model | Supply voltage | Logic voltage | Candidate pin | Function | Evidence | Risk / uncertainty |
| --- | --- | --- | ---: | ---: | --- | --- | --- | --- |
| Controller | NodeMCU 1.0 / ESP-12E | **NodeMCU 1.0 (ESP-12E Module), ESP8266** | Board power path not measured | 3.3 V GPIO | PlatformIO `nodemcuv2` | Local controller and web server | Owner's authoritative correction; PlatformIO board definition | Exact board power input and installed flash have not been measured |
| Motor driver | BTS7960 / IBT-2-style module | Unverified | Motor/logic rails unmeasured | Actual module threshold unmeasured | D5/GPIO14 RPWM and D6/GPIO12 LPWM are repository leads | Bidirectional DC motor drive | Current repository and owner recollection | Terminal order, module variant, cooling, current capacity, and 3.3 V acceptance need physical checks |
| Driver enable | R_EN and L_EN | Unverified | Repository claims 5 V logic | ESP8266 GPIO must not receive 5 V | External HIGH is the repository lead; optional D2/GPIO4 profile remains disabled | Bridge enable/inhibit | Commit `18a98b3` says both are pulled to 5 V | Firmware cannot prove either wire; joining a GPIO to an existing 5 V connection would damage the ESP8266 |
| Rain module | Digital rain sensor using DO | Unverified | Unmeasured | DO must stay within 0–3.3 V at the ESP8266 | D1/GPIO5 is the repository lead | Binary wet/dry input | Commit `ae752ab` and current wiring text | Model, VCC, DO pull-up voltage, polarity, threshold, and wet reliability are unknown |
| Actuator | DC motor; 12 V is recollected | Unverified | Unmeasured | n/a | BTS7960 motor outputs | Move shield | Repository text and owner recollection only | Rated voltage, direction, rated/stall current, travel time, jam behavior, and endpoints are unknown |
| Retracted limit | None evidenced | Unverified / probably absent | 3.3 V candidate | Active-low candidate | Optional D7/GPIO13, disabled | Confirm shield retracted | New optional profile only | Presence, contact type, placement, and polarity are unknown |
| Deployed limit | None evidenced | Unverified / probably absent | 3.3 V candidate | Active-low candidate | Optional D0/GPIO16 with external pull-up, disabled | Confirm shield deployed | New optional profile only | GPIO16 needs special care; presence, placement, and polarity are unknown |
| Power system | Old README named 12 V 10 A and LM2596 | Unverified | Unmeasured | n/a | n/a | Supply motor and logic | Stale prose only; no schematic, photo, label, or measurement | Capacity, regulation, fuse, wire size, connector rating, grounding, noise, and weather protection are unknown |

The controller is the only physically authoritative model in this table. Repository evidence is not physical confirmation.

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
- The rain module is unidentified. Some comparator boards pull DO toward their VCC. Powering a compatible module at 3.3 V may make DO safe, but threshold behavior must still be tested. If it requires 5 V, use a properly designed divider, buffer, or level shifter and measure the resulting HIGH level before connection.
- The BTS7960 silicon datasheet lists logic thresholds compatible with a 3.3 V HIGH, but that does not certify an unknown IBT-2 module, its extra components, terminal labels, or wiring. Verify the actual board. See the [Infineon BTS7960 datasheet](https://www.infineon.com/assets/row/public/documents/10/57/infineon-bts7960-ds-en.pdf).
- If R_EN/L_EN are really tied high, firmware can stop only by setting both PWM inputs LOW. If they are instead moved to D2, remove every 5 V enable connection first and set `CONTROL_BTS_ENABLE=true` only after checking the new wiring. Never combine the two arrangements.
- A separate motor power path and a regulated controller supply are expected. Their grounds normally need a common reference for logic signals, but shared ground is not confirmed on this build.
- Supply and fuse sizing must be based on measured motor stall current, not the headline current printed on a driver listing. Add a physical emergency disconnect, appropriately rated fuse, wire/connector sizing, strain relief, suppression, and a weatherproof enclosure.
- No current sensor is evidenced. Software cannot detect a jam or overload merely from elapsed time.

## Position truth

With `USE_LIMIT_SWITCHES=false`, reaching 0% or 100% means only that the configured time elapsed. The dashboard labels this `ESTIMATED`; it is not endpoint evidence. Voltage, load, friction, slipping, obstruction, and interrupted movement cause drift. STOP produces `STOPPED_PARTIAL`. A reboot after a saved active movement produces `UNKNOWN`. A manually observed endpoint is `USER_CALIBRATED`, while an enabled physical switch would be `LIMIT_CONFIRMED`.

The actual deploy direction, retract direction, endpoint behavior, and full-travel time remain unverified. `DEPLOY_USES_RPWM` is therefore a configurable assumption, not a physical fact.

## Scope tiers

Implementable with the believed hardware: local wet/dry filtering, AUTO/MANUAL, deploy/retract/STOP, reversal dead-time, maximum runtime, fault lockout, offline autonomy, local dashboard, dry-run, and explicitly estimated position.

Small hardware additions: two endpoint switches, current/stall sensing, correctly sized fuse, physical emergency disconnect, verified level shifting, improved regulated power, and weatherproofing.

Longer term: soil moisture, forecast-aware policy, battery/solar monitoring, notifications, history, and multiple zones. These are not part of this rebuild.
