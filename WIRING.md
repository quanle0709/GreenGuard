# Wiring worksheet — verify before connecting

> **This is not a confirmed as-built diagram.** NodeMCU 1.0 ESP-12E, HW-039/BTS7960, the 12 V geared motor, RainDrop sensor, 12 V 10 A supply, and XL4005 are confirmed. The exact pin-to-terminal wiring below remains a candidate until an as-built table is supplied. Keep the public repository at `ACTUATOR_DRY_RUN=true`.

## Candidate profile compiled by default

| NodeMCU label | ESP8266 GPIO | Candidate connection | Status | Electrical rule |
| --- | ---: | --- | --- | --- |
| D1 | 5 | RainDrop DO | Pin mapping unverified; signal measured | Owner measured 3.27 V dry / 0.08 V wet, active-LOW |
| D5 | 14 | HW-039/BTS7960 RPWM | Firmware mapping; terminal wire unverified | PWM output; prototype function confirmed |
| D6 | 12 | HW-039/BTS7960 LPWM | Firmware mapping; terminal wire unverified | PWM output; must never be active with RPWM |
| D2 | 4 | Optional shared R_EN/L_EN control | Disabled, not part of default wiring | Use only after removing all 5 V enable connections |
| D7 | 13 | Optional retracted limit | Disabled / no switch confirmed | External pull-up to 3.3 V in the proposed profile |
| D0 | 16 | Optional deployed limit | Disabled / no switch confirmed | External pull-up to 3.3 V required in the proposed profile |
| D3 / D4 / D8 | 0 / 2 / 15 | No GreenGuard connection | Intentionally avoided | ESP8266 boot-strapping pins |

The code assumes `DEPLOY` uses RPWM only as a configurable starting point. If physical motion is opposite, change `deployUsesRpwm`; do not rename the physical meanings.

## R_EN and L_EN decision

The exact R_EN/L_EN wiring was not included in the supplied validation record. If both enable inputs are pulled to the driver's 5 V logic supply, `CONTROL_BTS_ENABLE=false`, D2 stays unused, and firmware stops by setting RPWM=LPWM=0.

A later improvement may join R_EN and L_EN to D2 so firmware can inhibit the bridge. Do this only after disconnecting both enable pins from 5 V. Never connect a 5 V enable node to an ESP8266 GPIO. The actual module's input thresholds and terminal layout must be verified first.

## Power worksheet

| Path | Candidate topology | Still required |
| --- | --- | --- |
| Motor power | Confirmed 12 V 10 A supply → protection → driver B+/B− | Observed 12.18 V before movement / 11.72 V during movement; fuse and stall evidence still required |
| Motor output | Driver M+/M− → motor | Identify physical deploy/retract direction |
| Controller | Confirmed XL4005 step-down in the prototype | Minimum observed 3V3 was 3.17 V; exact supply path/ripple remain undocumented |
| Driver logic | Supply required by actual IBT-2 board | Read board markings/data and verify 3.3 V input recognition |
| Ground | Common logic reference is expected | Trace and measure shared ground before signal connection |
| Rain sensor | Confirmed RainDrop module | DO measured safe in the tested arrangement; re-measure after any supply/wiring change |

The 10 A supply rating is confirmed, but it does not determine the correct fuse by itself. Use stall-current evidence and the ratings of wire, connectors, switch, driver, and supply. Add a reachable physical emergency disconnect.

See [Hardware audit](docs/HARDWARE_AUDIT.md) and [hardware test checklist](docs/HARDWARE_TEST_CHECKLIST.md) before any powered test.
