# STM32CubeIDE Project

`Smart_Classroom_Occupancy` is the complete STM32CubeIDE project used for the prototype. It contains the CubeMX configuration, HAL and CMSIS drivers, X-CUBE-AI generated network, sensor drivers, and application source code.

## Open and build

1. Start STM32CubeIDE.
2. Import `Smart_Classroom_Occupancy` as an existing project.
3. Open `Project 1.ioc` if CubeMX asks to regenerate the configuration.
4. Select a Debug build and build the project.
5. Connect the NUCLEO-L476RG through ST-LINK, then flash and open the serial monitor.

The generated `Debug` directory is not committed because it only contains local compiler output. CubeIDE recreates it during the build.

## Important files

| Path | Purpose |
| --- | --- |
| `Project 1.ioc` | STM32CubeMX board and peripheral configuration. |
| `Core/Src/main.c` | Sensor reading, feature-window handling, user feedback, and inference flow. |
| `Core/Src/occupancy_ai.c` | Runtime wrapper for the X-CUBE-AI network. |
| `Core/Src/occupancy_features.c` | Feature normalisation and INT8 conversion values exported with the trained model. |
| `X-CUBE-AI/App` | Generated model source and generation report. |

The project retains the X-CUBE-AI licence supplied with the generated content.
