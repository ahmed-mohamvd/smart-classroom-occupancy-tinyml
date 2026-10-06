# Sensor Test Programs

These files are standalone hardware tests used for wiring checks and troubleshooting. They are not the final application firmware.

| File | Purpose |
|---|---|
| `I2C_ADDRESS_SCANNER_ONLY.txt` | Scans connected I2C device addresses |
| `BH1750_OLED_CODE.txt` | Tests BH1750 with the OLED on STM32 |
| `NodeMCU_BH1750_Test.ino` | Earlier BH1750 test using Arduino/ESP8266 |
| `VL53L0X_OLED_CODE.txt` | Tests VL53L0X distance readings and OLED output |
| `vl53l0x_platform_STM32_HAL.c.txt` | STM32 HAL platform layer for the VL53L0X library |
| `VL53L0X_STM32_SETUP.md` | Setup notes for the VL53L0X library on STM32 |
| `FULL_HARDWARE_TEST_WITHOUT_BH1750.txt` | Earlier full-system hardware test before BH1750 integration |

If the VL53L0X is replaced with a VL53L1X, use the correct driver for the new sensor even though both devices commonly use I2C address `0x29`.
