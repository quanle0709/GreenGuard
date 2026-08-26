# Hardware test checklist

No item below was checked during the software rebuild. Record measurements, date, operator, and evidence before changing `ACTUATOR_DRY_RUN`.

## Identify and measure with motor disconnected

- [ ] Photograph the controller markings and confirm NodeMCU 1.0 / ESP-12E.
- [ ] Record every actual wire in a pin-to-terminal table; do not rely on repository defaults.
- [ ] Confirm RPWM, LPWM, R_EN, L_EN, VCC, GND, B+, B−, M+, and M− labels on the actual driver.
- [ ] Measure rain-module VCC and DO in dry and wet states. DO at the ESP8266 must never exceed 3.3 V.
- [ ] Determine whether wet is DO LOW or HIGH and record the threshold adjustment.
- [ ] Confirm no ESP8266 GPIO has a 5 V or 12 V path.
- [ ] Confirm controller ground, driver logic ground, sensor ground, and motor-supply negative share the intended reference.
- [ ] Read the motor rated voltage/current label and measure or obtain its stall current.
- [ ] Verify power-supply voltage, continuous/current-surge capacity, regulation, and polarity under load.
- [ ] Fit a fuse sized from the measured circuit, plus a physical emergency disconnect.
- [ ] Check wire gauge, connector rating, strain relief, suppression, ventilation, and weatherproofing.
- [ ] Identify both endpoint switches if present; record normally-open/closed behavior and voltage.

## First powered logic test

- [ ] Keep the motor physically disconnected and `ACTUATOR_DRY_RUN=true`.
- [ ] Verify `board = nodemcuv2` and positively identify the intended serial port before flashing.
- [ ] Measure RPWM and LPWM at boot; both must remain LOW.
- [ ] If firmware-controlled enable is proposed, first remove every external 5 V connection from R_EN/L_EN and review the new wiring. Never join 5 V and D2.
- [ ] Observe serial boot, LittleFS, Wi-Fi retry, and offline AUTO without connecting the motor.
- [ ] Test dry/wet DO and measure the 3 s wet / 120 s dry defaults.
- [ ] Open the dashboard by IP and, if supported, `http://greenguard.local`.
- [ ] Test token rejection, AUTO, MANUAL, deploy, retract, STOP, reset, and stale/disconnect display in dry-run.

## Motor tests only after the previous sections pass

- [ ] Provide an unloaded, restrained mechanism and a reachable emergency disconnect.
- [ ] Change `ACTUATOR_DRY_RUN=false` in a reviewed local build only; never commit that default.
- [ ] Send one short low-duty pulse and identify which physical direction is DEPLOY.
- [ ] Test STOP during each direction.
- [ ] Measure that both PWM signals are LOW throughout reversal dead-time.
- [ ] Test each endpoint separately; confirm motor power stops at the actual endpoint.
- [ ] If switches exist, test each one and the both-active fault with motor power isolated where possible.
- [ ] If switches do not exist, measure full travel repeatedly under realistic load and record variation; treat position as estimated.
- [ ] Test obstruction/stall response without exceeding motor, driver, wire, connector, or supply ratings.
- [ ] Reinspect temperature, noise resets, rain-input interference, enclosure sealing, drainage, and corrosion protection.

## Record

| Item | Measured value / evidence | Date | Operator |
| --- | --- | --- | --- |
| Actual GPIO wiring | Not tested | — | — |
| Rain DO dry/wet voltage and polarity | Not tested | — | — |
| Motor rated/stall current | Not tested | — | — |
| Supply and fuse | Not tested | — | — |
| Deploy/retract direction | Not tested | — | — |
| Endpoints / switches | Not tested | — | — |
| Full-travel time | Not tested | — | — |
