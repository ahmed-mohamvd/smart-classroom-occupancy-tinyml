# Smart Classroom Occupancy Detection with TinyML on STM32

An end-to-end embedded AI prototype for **classroom occupancy estimation and energy-aware monitoring**, developed during the **VeCAD Elite Internship 2026**.

The system collects motion and environmental sensor data, trains a compact TensorFlow model for three occupancy classes, quantizes it to INT8, and prepares it for on-device inference on an **STM32 NUCLEO-L476RG** using **STM32Cube.AI / ST Edge AI**.

> **Occupancy classes:** `EMPTY` · `LOW` · `HIGH`

![System architecture](assets/system_architecture.png)

## Project Highlights

- **Embedded target:** STM32 NUCLEO-L476RG
- **TinyML task:** 3-class occupancy classification
- **AI input:** 6 motion features derived from 2 PIR sensors
- **Window:** 30 samples, with a new decision every 5 samples
- **Test accuracy:** **88.67%**
- **Macro F1:** **87.47%**
- **INT8 TFLite model:** **3,616 bytes**
- **STM32Cube.AI inference footprint:** 356-byte weights, 1,028-byte activation RAM, 320 MACC/inference
- **Additional sensing:** BME280, BH1750, VL53L0X, OLED, LEDs and buzzer
- **Data pipeline:** sensor acquisition → CSV logging → cleaning → feature windows → TensorFlow training → INT8 quantization → STM32 deployment

## System Overview

The classifier intentionally uses only the two PIR sensors as model inputs. The other sensors provide environmental and distance context for monitoring and warning rules.

![Project introduction](assets/project_introduction.png)

### Hardware

| Component | Purpose |
|---|---|
| STM32 NUCLEO-L476RG | Main embedded controller |
| 2 × PIR sensors | Motion features used by the AI model |
| BME280 | Temperature, humidity and pressure |
| BH1750 | Ambient light measurement |
| VL53L0X | Distance/context sensing |
| OLED display | Live measurements and AI result |
| HC-06 | Optional Bluetooth data link |
| LEDs + active buzzer | Local status and warning feedback |

## Machine Learning Results

The selected model uses six PIR-derived features:

`pir_1_ratio`, `pir_2_ratio`, `pir_any_ratio`, `pir_both_ratio`, `pir_1_rising_edges`, `pir_2_rising_edges`.

| Split | Accuracy | Macro F1 |
|---|---:|---:|
| Train | 87.25% | 88.44% |
| Validation | 81.37% | 83.18% |
| Test | **88.67%** | **87.47%** |
| INT8 test | **88.67%** | **87.47%** |

![Training curves](assets/training_curves.png)

![Confusion matrix](assets/confusion_matrix.png)

Detailed metrics are stored in `ai_module/tensorflow_occupancy/artifacts_pir_v2/metrics.json`.

## Repository Structure

```text
.
├── assets/                     # Images used in this README
├── firmware/                   # STM32 reference firmware and sensor tests
├── data_collection/            # Serial logger, raw data, cleaning and planning
├── ai_module/
│   └── tensorflow_occupancy/   # Feature engineering, training and model artifacts
├── stm32_ai_integration/       # TinyML preprocessing and STM32Cube.AI integration
├── docs/                       # Selected portfolio documentation
└── README.md                   # Project overview
```

## Data Collection

The STM32 records synchronized sensor readings once per second. A Python serial logger saves the readings into CSV files, while occupancy labels are assigned as `EMPTY`, `LOW`, or `HIGH`.

Install the logger dependency:

```bash
python -m pip install -r data_collection/requirements.txt
```

Example serial logging command:

```bash
python data_collection/serial_data_logger.py --port COM8 --baud 115200
```

Raw recordings are preserved in `data_collection/collected_data/raw/`. The cleaned training dataset is available at:

```text
data_collection/collected_data/processed/occupancy_training_dataset_v7.csv
```

## Training Pipeline

Build 30-sample windows:

```bash
python ai_module/tensorflow_occupancy/build_window_dataset.py \
  data_collection/collected_data/processed/occupancy_training_dataset_v7.csv \
  --output ai_module/tensorflow_occupancy/window_features_v2.csv \
  --window-size 30 --step-size 5
```

Train the selected PIR model:

```bash
python ai_module/tensorflow_occupancy/train_tensorflow.py \
  ai_module/tensorflow_occupancy/window_features_v2.csv \
  --artifacts ai_module/tensorflow_occupancy/artifacts_pir_v2 \
  --feature-set pir \
  --split-config ai_module/tensorflow_occupancy/split_config_v2.csv
```

The main deployment artifact is:

```text
ai_module/tensorflow_occupancy/artifacts_pir_v2/occupancy_model_int8.tflite
```

## STM32 TinyML Integration

`stm32_ai_integration/` contains the preprocessing code and a reference copy of the network generated with STM32Cube.AI / ST Edge AI.

At runtime, the MCU:

1. Collects the latest 30 PIR samples.
2. Computes the six model features.
3. Normalizes and quantizes the feature vector to INT8.
4. Runs the neural-network inference locally.
5. Selects `EMPTY`, `LOW`, or `HIGH` from the model output.
6. Displays the result and uses environmental context for local warning logic.

## Important Firmware Note

`firmware/current/main_pir_filtered.c` is the latest data-collection `main` snapshot stored in this repository. The complete final STM32CubeIDE project that contained the fully integrated AI application is **not** included here, so this file should not be copied over a working CubeIDE `Core/Src/main.c` without comparison.

## Portfolio Documentation

Selected supporting material is available in `docs/`, including the final presentation and the 10-day project diary.

Local IDE metadata, Jupyter runtime state, temporary files, caches, duplicate presentation revisions and non-English helper notes were intentionally excluded from this public-ready version.

## Author

**Ahmed Mohamed Abdalla Ahmed**  
Electrical Engineering student  
GitHub: [@ahmed-mohamvd](https://github.com/ahmed-mohamvd)

## Notes on Licensing

This repository contains original project work together with generated STM32Cube.AI files. Generated/third-party components retain their own license terms; see `stm32_ai_integration/generated_network/LICENSE.txt` where applicable.
