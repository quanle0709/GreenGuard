# Hardware validation and reproduction checklist

This record separates what we verified on our completed prototype on **June 26, 2026** from checks another builder should repeat and from safety questions outside our ten-cycle validation. The public repository remains at `ACTUATOR_DRY_RUN=true`.

## Verified on our prototype on June 26, 2026

- [x] NodeMCU 1.0 ESP-12E / ESP8266 controller used with the updated working firmware.
- [x] BTS7960 / HW-039 drove the real 12 V geared-motor mechanism.
- [x] RainDrop DO was connected to D1/GPIO5 and behaved active-LOW.
- [x] BTS7960 RPWM was connected to D5/GPIO14.
- [x] BTS7960 LPWM was connected to D6/GPIO12.
- [x] BTS7960 R_EN and L_EN control was connected to D2/GPIO4.
- [x] The retracted limit-switch connection was wired to D7/GPIO13.
- [x] The deployed limit-switch connection was wired to D0/GPIO16.
- [x] The completed mechanism performed 10 consecutive loaded deploy–retract cycles.
- [x] RPWM/LPWM reversal dead-time measured 307 ms against the configured 300 ms target.
- [x] The NodeMCU did not reset during the observed motor-startup events.

| Measurement | Recorded result |
| --- | ---: |
| Motor supply before / during movement | 12.18 V / 11.72 V |
| Supply voltage drop | 3.8% |
| Lowest 3V3 rail during motor startup | 3.17 V |
| RainDrop DO dry / wet | 3.27 V / 0.08 V, active-LOW |
| Loaded running motor current | 2.63 A |
| Maximum measured startup current | 7.20 A |
| Reversal dead-time | 307 ms |
| Highest motor temperature after 10 cycles | 51°C |
| Wire and connector temperature after 10 cycles | 34°C |
| Evaluation | **PASS with conditions within this test scope** |

These results describe our tested arrangement and load. The actuator-enabled configuration used for the physical test was local and is not the committed public default.

## Recommended before reproducing or modifying the system

### With motor power disconnected

- [ ] Confirm the board markings and select PlatformIO `board = nodemcuv2`.
- [ ] Compare every wire against the [confirmed as-built table](../WIRING.md); record any intentional change.
- [ ] Confirm RPWM, LPWM, R_EN, L_EN, VCC, GND, B+, B−, M+, and M− labels on the specific driver module.
- [ ] Measure rain-module VCC and DO in dry and wet states. DO at the ESP8266 must remain within the 3.3 V GPIO range.
- [ ] Confirm no ESP8266 GPIO has a 5 V or 12 V path.
- [ ] Trace and measure the intended shared reference between controller, driver logic, sensor, and motor-supply negative.
- [ ] Record the motor's rated voltage and current, then obtain or safely measure stall current.
- [ ] Select a fuse, wire gauge, connectors, and emergency disconnect from the measured circuit requirements.
- [ ] Check strain relief, suppression, ventilation, drainage, and weather protection.
- [ ] Characterize both limit switches: contact type, voltage, pull-up requirements, active polarity, placement, and repeatability.

### First powered logic test

- [ ] Keep the motor physically disconnected and `ACTUATOR_DRY_RUN=true`.
- [ ] Positively identify the intended serial port before flashing.
- [ ] Measure RPWM and LPWM at boot and during test commands; both must remain LOW in dry-run.
- [ ] Leave `CONTROL_BTS_ENABLE=false` until D2 and the R_EN/L_EN node have been measured and reviewed. Never share that GPIO node with an external 5 V source.
- [ ] Leave `USE_LIMIT_SWITCHES=false` until D7/D0 voltage, polarity, pull-ups, and endpoint behavior are verified.
- [ ] Observe serial boot, LittleFS, Wi-Fi retry, and offline AUTO behavior.
- [ ] Test RainDrop DO, token rejection, AUTO, MANUAL, deploy, retract, STOP, reset, and stale/disconnect display in dry-run.

### Controlled motor test

- [ ] Use a restrained mechanism, a reachable physical disconnect, and an operator watching the test.
- [ ] Use an actuator-enabled configuration only in a reviewed local build; never commit it as the default.
- [ ] Confirm which terminal polarity and PWM direction physically deploys and retracts the shield.
- [ ] Test STOP during each direction and confirm both PWM signals remain LOW throughout reversal dead-time.
- [ ] After electrical characterization, test each limit switch separately and test the both-active fault with motor power isolated where possible.
- [ ] Measure full-travel time repeatedly under realistic load and record its variation.
- [ ] Design a safe obstruction/stall test that cannot exceed motor, driver, wire, connector, or supply ratings.
- [ ] Reinspect temperature, noise resets, sensor interference, enclosure sealing, drainage, and corrosion protection.

## Not covered by our ten-cycle validation

- Limit-switch contact polarity, calibration, repeatability, and endpoint accuracy.
- Exact full-travel time and variation across voltage, load, and temperature.
- Motor stall current and every possible jam or obstruction condition.
- Final fuse selection, wire gauge, connector margin, and emergency-disconnect rating.
- Complete shared-ground and controller-power topology measurements.
- Long-term endurance, product-lifetime reliability, outdoor weather resistance, ingress protection, drainage, and corrosion resistance.
