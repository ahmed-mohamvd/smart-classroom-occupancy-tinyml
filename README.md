# TinyML Classroom Occupancy Estimation

An embedded classroom-occupancy prototype for the VeCAD Elite Internship 2026. The system collects motion and environmental readings, estimates occupancy locally on an STM32 NUCLEO-L476RG, and provides a low-voltage energy-awareness reminder when the room appears empty while light remains on.

## Project outcome

The deployed classifier uses six features derived from two PIR sensors and predicts three occupancy classes:

- **EMPTY**: no person in the room
- **LOW**: one person in the room
- **HIGH**: two or three people in the room

The final INT8 TensorFlow Lite model achieved **88.67% test accuracy** and **87.47% macro F1** on 256 windows from recording sessions not used during training. The model file is 3,616 bytes. STM32Cube.AI analysis reports 320 MACC per inference, 13.60 KiB Flash, and 2.91 KiB RAM.

## Repository structure

| Path | Contents |
| --- | --- |
| `firmware/stm32cubeide/Smart_Classroom_Occupancy` | Complete STM32CubeIDE project, including the CubeMX `.ioc` configuration, STM32 drivers, X-CUBE-AI generated network, and application source code. |
| `assets` | Visual assets retained from the initial repository upload. |
| `firmware/current` | Reference firmware saved during the project. |
| `firmware/sensor_tests` | Individual sensor and bus-test code. |
| `data_collection` | Serial logger, cleaning script, session plan, raw labelled CSV sessions, and processed training dataset. |
| `ai_module/tensorflow_occupancy` | Window-feature generation, TensorFlow training code, split configuration, trained model artifacts, and recorded evaluation results. |
| `stm32_ai_integration` | Feature normalisation, quantisation helpers, and generated STM32Cube.AI network files. |
| `documentation` | Internship documentation, logbooks, project reference material, and presentation assets. |
| `deliverables/final_presentation` | Final presentation PDF, editable PowerPoint source, and the printable ten-minute presentation script. |

The earlier Arabic project guides remain available as `README_AR.md` files in the relevant folders.

## System overview

- Two PIR sensors provide the six motion features used by the classifier.
- BME280, BH1750, and VL53L0X readings support environmental monitoring, display, and warning rules. They are not model inputs in the final classifier.
- The model uses a 30-sample motion window and updates its result every five samples after the first full window.
- The NUCLEO-L476RG runs the quantised model locally and sends the result to the OLED, LEDs, buzzer, and serial monitor.

## Data and model

The final cleaned dataset contains 6,479 accepted sensor readings from 13 labelled sessions. Training keeps whole recording sessions separate across training, validation, and test data to avoid leakage. The neural network architecture is `6 -> 16 ReLU -> 8 ReLU -> 3 Softmax`, with 275 trainable parameters.

Model inputs, in order:

1. PIR 1 active ratio
2. PIR 2 active ratio
3. Any-PIR active ratio
4. Both-PIR active ratio
5. PIR 1 rising-edge count
6. PIR 2 rising-edge count

## Running the software

### Data collection and training

Install the requirements listed in `data_collection/requirements.txt` and `ai_module/tensorflow_occupancy/requirements.txt`. The scripts and their Arabic usage notes are stored beside the data and model artifacts.

### STM32 firmware

Open `firmware/stm32cubeide/Smart_Classroom_Occupancy` as an existing STM32CubeIDE project. The `.ioc` file targets the NUCLEO-L476RG. The generated `Debug` build directory is deliberately not tracked; build the project locally after importing it.

## Safety scope

This prototype provides low-voltage local feedback only. It does not switch building mains, lighting circuits, or air-conditioning equipment directly.

## Third-party components

The STM32CubeIDE project includes STMicroelectronics-generated X-CUBE-AI content and drivers. Their original licence files are retained in the project tree, including `LICENSE_X-CUBE-AI.txt`.
