# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-10-15

### Added
- Initial release of the heart-rate monitor:
  - Arduino firmware (`firmware/heart_sensor_BMI/heart_sensor_BMI.ino`) reading
    the MAX30102 over I2C, with beat detection, an 8-beat BPM moving average,
    finger detection, SSD1306 OLED output and a beat-synchronised LED on D9.
- `docs/wiring.md` — bill of materials with Fritzing search terms, full pin
  map for the MAX30102 / SSD1306 / LED, shared I2C bus notes, power budget and
  an assembly checklist.
- `docs/investigation.md` — student lab guide covering PPG theory, a four-part
  experimental procedure, observation tables, analysis questions (including
  SpO2 and BMI extensions), safety notes and troubleshooting.
- `hardware/` reference files: Fritzing breadboard / schematic / PCB exports
  (`poi.png`, `Schematic_poi.png`, `PCB_poi.png`), the `gy-max30102-module.fzpz`
  Fritzing part, and a physical setup video (`SetupVideo.mov`).
- Project documentation: README, CHANGELOG and MIT LICENSE.
