# Selected PIR Model Artifacts — v2

| File | Purpose |
|---|---|
| `occupancy_model_int8.tflite` | Quantized model used for STM32 deployment |
| `occupancy_model_float.tflite` | Floating-point TFLite comparison model |
| `occupancy_model.keras` | Original TensorFlow/Keras model |
| `feature_config.json` | Feature order plus normalization/quantization constants |
| `metrics.json` | Accuracy, F1 and class-level metrics |
| `confusion_matrix_*.csv` | Confusion matrices for train/validation/test |
| `split_summary.csv` | Sample/window counts by split and class |
| `test_predictions.csv` | Predictions for the held-out test set |
| `training_history.csv` | Epoch-by-epoch training history |

This is the model version used by the STM32 integration. If the model is retrained, regenerate the STM32Cube.AI network and keep the preprocessing constants synchronized.
