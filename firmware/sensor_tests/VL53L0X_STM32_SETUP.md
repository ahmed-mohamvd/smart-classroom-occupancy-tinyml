# VL53L0X + OLED STM32 setup

The address test has already confirmed:

- VL53L0X: 7-bit address `0x29` (`0x52` in STM32 HAL calls)
- SSD1306 OLED: 7-bit address `0x3C` (`0x78` in STM32 HAL calls)

## Files required from STSW-IMG005

Download the official VL53L0X API package from STMicroelectronics. Add these
official API source files to the STM32CubeIDE build:

- `vl53l0x_api.c`
- `vl53l0x_api_calibration.c`
- `vl53l0x_api_core.c`
- `vl53l0x_api_ranging.c`
- `vl53l0x_api_strings.c`

Add the matching header files from the same API release to the compiler include
path, including:

- `vl53l0x_api.h`
- `vl53l0x_api_calibration.h`
- `vl53l0x_api_core.h`
- `vl53l0x_api_ranging.h`
- `vl53l0x_api_strings.h`
- `vl53l0x_def.h`
- `vl53l0x_device.h`
- `vl53l0x_interrupt_threshold_settings.h`
- `vl53l0x_platform.h`
- `vl53l0x_platform_log.h`
- `vl53l0x_tuning.h`
- `vl53l0x_types.h`

Rename `vl53l0x_platform_STM32_HAL.c.txt` to `vl53l0x_platform.c` and add it to
the build. Use `VL53L0X_OLED_CODE.txt` as `main.c`.

Do not mix API files from different releases. Keep the existing `ssd1306` and
font files in the project.

## Wiring

```text
VL53L0X VIN -> 3.3 V
VL53L0X GND -> GND
VL53L0X SCL -> I2C1 SCL (shared with OLED)
VL53L0X SDA -> I2C1 SDA (shared with OLED)
```

## Expected UART startup

Each initialization step should report `API status=0`, followed by:

```text
VL53L0X initialization OK.
Distance=... mm, RangeStatus=0
```

`RangeStatus=0` means the range is valid. A nonzero range status means the API
communicated with the sensor but rejected that particular measurement.
