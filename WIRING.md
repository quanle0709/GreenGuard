# Wiring worksheet — verify before connecting

> **This is not a confirmed as-built diagram.** The controller is confirmed as NodeMCU 1.0 ESP-12E, but every signal and power wire below remains a candidate until physically traced and measured. Keep motor power disconnected and keep `ACTUATOR_DRY_RUN=true`.

## Candidate profile compiled by default

| NodeMCU label | ESP8266 GPIO | Candidate connection | Status | Electrical rule |
| --- | ---: | --- | --- | --- |
| D1 | 5 | Rain module DO | Repository lead, unverified | Measure LOW/HIGH voltage; never exceed 3.3 V |
| D5 | 14 | BTS7960 RPWM | Repository lead, unverified | PWM output; confirm module accepts 3.3 V HIGH |
| D6 | 12 | BTS7960 LPWM | Repository lead, unverified | PWM output; must never be active with RPWM |
| D2 | 4 | Optional shared R_EN/L_EN control | Disabled, not part of default wiring | Use only after removing all 5 V enable connections |
| D7 | 13 | Optional retracted limit | Disabled / no switch confirmed | External pull-up to 3.3 V in the proposed profile |
| D0 | 16 | Optional deployed limit | Disabled / no switch confirmed | External pull-up to 3.3 V required in the proposed profile |
| D3 / D4 / D8 | 0 / 2 / 15 | No GreenGuard connection | Intentionally avoided | ESP8266 boot-strapping pins |

The code assumes `DEPLOY` uses RPWM only as a configurable starting point. If physical motion is opposite, change `deployUsesRpwm`; do not rename the physical meanings.

## R_EN and L_EN decision

The repository claims both enable inputs are pulled to the driver's 5 V logic supply, but this was not inspected. In that arrangement `CONTROL_BTS_ENABLE=false`, D2 stays unused, and firmware stops by setting RPWM=LPWM=0.

A later improvement may join R_EN and L_EN to D2 so firmware can inhibit the bridge. Do this only after disconnecting both enable pins from 5 V. Never connect a 5 V enable node to an ESP8266 GPIO. The actual module's input thresholds and terminal layout must be verified first.

## Power worksheet

| Path | Candidate topology | Still required |
| --- | --- | --- |
| Motor power | Rated motor supply → fuse/disconnect → driver B+/B− | Measure motor voltage/stall current and size every part |
| Motor output | Driver M+/M− → motor | Identify physical deploy/retract direction |
| Controller | Regulated USB/5 V board input | Verify the exact supply path and ripple; never apply 12 V to a 3.3 V/GPIO pin |
| Driver logic | Supply required by actual IBT-2 board | Read board markings/data and verify 3.3 V input recognition |
| Ground | Common logic reference is expected | Trace and measure shared ground before signal connection |
| Rain sensor | 3.3 V only if the actual module works reliably there; otherwise level-shift DO | Measure DO under every supply arrangement |

Do not select a fuse from a guessed “10 A supply” claim. Use measured motor stall current and the ratings of wire, connectors, switch, driver, and supply. Add a reachable physical emergency disconnect.

See [Hardware audit](docs/HARDWARE_AUDIT.md) and [hardware test checklist](docs/HARDWARE_TEST_CHECKLIST.md) before any powered test.
