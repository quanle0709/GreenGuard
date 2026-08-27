# GreenGuard hardware record

We completed the physical prototype and performed the recorded functional and electrical validation on **June 26, 2026**. This document separates our confirmed hardware and wiring from safe public firmware defaults and from behavior that still needs broader testing.

## Confirmed hardware and observations

| Component | Confirmed model or connection | Our physical evidence | Scope still to characterize |
| --- | --- | --- | --- |
| Controller | **NodeMCU 1.0 (ESP-12E Module), ESP8266**; PlatformIO `nodemcuv2` | Lowest observed 3V3 rail was 3.17 V during motor startup; no reset during 10 loaded cycles | Complete controller power topology, ripple, and regulator thermal margin |
| Motor driver | **BTS7960 / HW-039** | D5/GPIO14 to RPWM and D6/GPIO12 to LPWM; drove the loaded motor through 10 deploy–retract cycles | Cooling margin, obstruction behavior, and terminal polarity after any rewiring |
| Driver enable | D2/GPIO4 connected to BTS7960 R_EN and L_EN control | Confirmed as-built connection | Enable-node voltage and behavior were not included in our measurement set; public `CONTROL_BTS_ENABLE=false` |
| Rain module | **RainDrop digital sensor**, DO connected to D1/GPIO5 | 3.27 V dry, 0.08 V wet, active-LOW | Long-term wet exposure and behavior after supply or threshold changes |
| Actuator | **12 V DC geared motor** | 2.63 A loaded running current; 7.20 A maximum measured startup current; 10 loaded cycles | Stall current, jam response, full-travel variation, and lifetime endurance |
| Retracted limit connection | D7/GPIO13 | Confirmed as-built wire connection | Contact type, electrical polarity, calibration, and endpoint accuracy; public `USE_LIMIT_SWITCHES=false` |
| Deployed limit connection | D0/GPIO16 | Confirmed as-built wire connection | Contact type, electrical polarity, calibration, and endpoint accuracy; public `USE_LIMIT_SWITCHES=false` |
| Power system | **12 V, 10 A supply and XL4005 step-down module** | Motor rail was 12.18 V before and 11.72 V during movement, a 3.8% drop | Shared-ground record, final fuse, wire gauge, connector ratings, ripple, and weather protection |

## Confirmed as-built GPIO map

| NodeMCU pin | GPIO | Physical connection |
| --- | ---: | --- |
| D1 | GPIO5 | RainDrop DO |
| D5 | GPIO14 | BTS7960 RPWM |
| D6 | GPIO12 | BTS7960 LPWM |
| D2 | GPIO4 | BTS7960 R_EN and L_EN control |
| D7 | GPIO13 | Retracted limit-switch connection |
| D0 | GPIO16 | Deployed limit-switch connection |

These are the connections used in our completed prototype. Disabled firmware flags do not make the physical wiring optional or hypothetical.

## Physical validation summary

On June 26, 2026, our prototype completed 10 consecutive deploy–retract cycles with the real mechanical load. We measured 307 ms of reversal dead-time against the configured 300 ms target. Peak temperatures after the cycles were 51°C at the motor and 34°C at the wires and connectors. The result was **PASS with conditions within this test scope**.

The observed 3V3 rail remained stable enough during the measured startup events, and the NodeMCU did not reset. The test confirms the listed operation and measurements for those 10 cycles. It does not establish long-term endurance, outdoor weather resistance, ingress protection, corrosion resistance, every obstruction or stall condition, or product-lifetime reliability.

The real prototype used an actuator-enabled local configuration. The public repository intentionally retains `ACTUATOR_DRY_RUN=true`, `CONTROL_BTS_ENABLE=false`, and `USE_LIMIT_SWITCHES=false`.

## ESP8266 pin and boot audit

`platformio.ini` uses `board = nodemcuv2`. The firmware uses ESP8266 Arduino APIs, ESP8266WebServer, mDNS, and LittleFS.

The NodeMCU labels map as follows: D0→GPIO16, D1→GPIO5, D2→GPIO4, D3→GPIO0, D4→GPIO2, D5→GPIO14, D6→GPIO12, D7→GPIO13, and D8→GPIO15. D3/GPIO0, D4/GPIO2, and D8/GPIO15 participate in boot selection; GreenGuard assigns no sensor, PWM, enable, or limit function to those pins. The confirmed D0/GPIO16 limit connection requires careful electrical review because GPIO16 differs from the other GPIOs and does not provide the same normal internal pull-up behavior.

Sources:

- [PlatformIO NodeMCU 1.0 board definition](https://docs.platformio.org/en/latest/boards/espressif8266/nodemcuv2.html)
- [ESP8266 Arduino core documentation](https://arduino-esp8266.readthedocs.io/en/stable/)
- [Espressif boot-mode selection](https://docs.espressif.com/projects/esptool/en/latest/esp8266/advanced-topics/boot-mode-selection.html)
- [Espressif ESP8266EX datasheet](https://www.espressif.com/sites/default/files/documentation/0a-esp8266ex_datasheet_en.pdf)
- [Infineon BTS7960 datasheet](https://www.infineon.com/assets/row/public/documents/10/57/infineon-bts7960-ds-en.pdf)

## Electrical safety findings

- Treat every ESP8266 GPIO as 3.3 V-only. Never connect a 5 V or 12 V source to RainDrop DO, D2, D7, D0, or any other GPIO.
- RainDrop DO measured 3.27 V dry and 0.08 V wet in our tested arrangement. Recheck these levels after any sensor-supply, threshold, or wiring change.
- Our prototype connects both driver enable controls to D2/GPIO4. The public build does not drive D2 because `CONTROL_BTS_ENABLE=false`. Before enabling that option, measure the node and ensure that no external 5 V source shares it.
- The public build does not read the physically connected limit inputs because `USE_LIMIT_SWITCHES=false`. Do not enable them until contact polarity, pull-up requirements, and endpoint behavior have been verified.
- The observed motor rail fell from 12.18 V to 11.72 V, while the lowest observed 3V3 rail was 3.17 V without a NodeMCU reset. These observations do not replace a complete power and grounding diagram.
- Size the fuse and wiring from a circuit review and stall-current evidence, not the 7.20 A startup maximum or the headline current printed on the motor driver.
- Add or verify a physical emergency disconnect, strain relief, suppression, ventilation, and a weather-resistant enclosure before broader use.
- No current sensor is implemented, so firmware cannot detect a jam or overload from current.

## Position confidence

The physical D7 and D0 limit connections are confirmed, but the public build keeps `USE_LIMIT_SWITCHES=false`. It therefore infers 0% and 100% from configured travel time rather than switch input. The dashboard labels timed completion as `ESTIMATED`; STOP produces `STOPPED_PARTIAL`, and rebooting after saved movement produces `UNKNOWN`.

Our validation confirmed successful physical deploy–retract movement, but it did not record limit-switch polarity, switch calibration, endpoint accuracy, the exact terminal polarity for each physical direction, or full-travel time. Those behaviors must be measured before enabling switch-based confirmation or relying on timed position as a precise endpoint.
