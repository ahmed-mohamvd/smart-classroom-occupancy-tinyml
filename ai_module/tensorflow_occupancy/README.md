# TensorFlow Occupancy Classifier

This folder contains the active training pipeline for the TinyML occupancy classifier. The model predicts three classes: `EMPTY`, `LOW` and `HIGH`.

## Pipeline

1. Build 30-sample windows from the cleaned time-series dataset.
2. Extract six PIR-derived features.
3. Split data by session to reduce leakage.
4. Train the TensorFlow model.
5. Quantize to INT8 for embedded deployment.
6. Export artifacts for STM32Cube.AI / ST Edge AI.

## Build Window Features

```bash
python build_window_dataset.py ../../data_collection/collected_data/processed/occupancy_training_dataset_v7.csv \
  --output window_features_v2.csv --window-size 30 --step-size 5
```

## Train the Selected Model

```bash
python train_tensorflow.py window_features_v2.csv \
  --artifacts artifacts_pir_v2 \
  --feature-set pir \
  --split-config split_config_v2.csv
```

The selected PIR-only configuration achieved approximately **88.67% test accuracy** and **87.47% macro F1**. The INT8 model preserved the same reported test accuracy while remaining small enough for STM32 deployment.
