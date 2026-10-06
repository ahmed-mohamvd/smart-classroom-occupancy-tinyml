# STM32 Firmware Reference

This folder contains reference STM32 firmware files and standalone sensor-test code. The complete STM32CubeIDE project is not included here, so these files are intended for review and reuse rather than direct building from this folder.

- `current/` — latest saved `main` reference used for data collection.
- `sensor_tests/` — I2C scanner and individual sensor tests.

When updating the real CubeIDE project, merge the required logic into the generated `USER CODE` sections rather than replacing CubeMX-generated files blindly.
