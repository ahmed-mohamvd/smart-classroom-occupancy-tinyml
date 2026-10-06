"""Train and export a small TensorFlow occupancy classifier for STM32Cube.AI."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import pandas as pd
import tensorflow as tf

from build_window_dataset import ENVIRONMENT_FEATURES, PRESENCE_FEATURES


CLASS_NAMES = ["EMPTY", "LOW", "HIGH"]
PIR_FEATURES = PRESENCE_FEATURES[:6]
RELATIVE_ENVIRONMENT_FEATURES = [
    "temperature_delta",
    "humidity_delta",
    "pressure_delta",
    "light_delta",
    "light_std",
]
FEATURE_SETS = {
    "pir": PIR_FEATURES,
    "presence": PRESENCE_FEATURES,
    "relative": PRESENCE_FEATURES + RELATIVE_ENVIRONMENT_FEATURES,
    "all": PRESENCE_FEATURES + ENVIRONMENT_FEATURES,
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("windows", type=Path, help="Window feature CSV")
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--feature-set", choices=sorted(FEATURE_SETS), default="presence")
    parser.add_argument(
        "--split-config",
        type=Path,
        help=(
            "Optional CSV with recording_id and split columns. This keeps "
            "train/validation/test sessions explicit and reproducible."
        ),
    )
    parser.add_argument("--epochs", type=int, default=300)
    parser.add_argument("--batch-size", type=int, default=32)
    parser.add_argument("--seed", type=int, default=2026)
    return parser.parse_args()


def assign_recording_splits(
    data: pd.DataFrame,
    split_config: Path | None = None,
) -> pd.DataFrame:
    output = data.copy()
    if split_config is not None:
        if not split_config.exists():
            raise FileNotFoundError(split_config)
        config = pd.read_csv(split_config)
        required = ["recording_id", "split"]
        missing_columns = [name for name in required if name not in config.columns]
        if missing_columns:
            raise ValueError(
                f"{split_config}: missing required columns {missing_columns}"
            )
        config = config[required].copy()
        config["recording_id"] = config["recording_id"].astype(str).str.strip()
        config["split"] = config["split"].astype(str).str.strip().str.lower()
        if config["recording_id"].duplicated().any():
            raise ValueError(f"{split_config}: recording_id values must be unique")
        invalid_splits = sorted(set(config["split"]) - {"train", "validation", "test"})
        if invalid_splits:
            raise ValueError(f"{split_config}: invalid split values {invalid_splits}")

        available = set(output["recording_id"].astype(str).unique())
        configured = set(config["recording_id"])
        missing_recordings = sorted(available - configured)
        unknown_recordings = sorted(configured - available)
        if missing_recordings or unknown_recordings:
            raise ValueError(
                f"{split_config}: missing recordings {missing_recordings}; "
                f"unknown recordings {unknown_recordings}"
            )
        output = output.merge(config, on="recording_id", how="left", validate="many_to_one")
        coverage = (
            output.groupby(["split", "occupancy_label"])["recording_id"]
            .nunique()
            .unstack(fill_value=0)
            .reindex(index=["train", "validation", "test"], fill_value=0)
            .reindex(columns=CLASS_NAMES, fill_value=0)
        )
        missing_class_splits = [
            f"{split}/{class_name}"
            for split in coverage.index
            for class_name in coverage.columns
            if int(coverage.loc[split, class_name]) == 0
        ]
        if missing_class_splits:
            raise ValueError(
                "Every split needs all three classes; missing "
                + ", ".join(missing_class_splits)
            )
        return output

    output["split"] = "train"
    recordings = (
        output.groupby("recording_id", as_index=False)
        .agg(
            class_id=("class_id", "first"),
            first_window=("window_start", "min"),
        )
        .sort_values("first_window")
    )
    for class_id, class_name in enumerate(CLASS_NAMES):
        names = recordings.loc[
            recordings["class_id"].eq(class_id), "recording_id"
        ].tolist()
        if len(names) < 4:
            raise ValueError(
                f"{class_name} needs at least four independent recordings"
            )
        output.loc[output["recording_id"].eq(names[-2]), "split"] = "validation"
        output.loc[output["recording_id"].eq(names[-1]), "split"] = "test"
    return output


def classification_metrics(
    actual: np.ndarray,
    predicted: np.ndarray,
) -> tuple[dict[str, object], np.ndarray]:
    matrix = np.zeros((len(CLASS_NAMES), len(CLASS_NAMES)), dtype=int)
    for expected, result in zip(actual, predicted):
        matrix[int(expected), int(result)] += 1

    per_class: dict[str, object] = {}
    f1_values: list[float] = []
    for class_id, class_name in enumerate(CLASS_NAMES):
        true_positive = int(matrix[class_id, class_id])
        false_positive = int(matrix[:, class_id].sum() - true_positive)
        false_negative = int(matrix[class_id, :].sum() - true_positive)
        precision = true_positive / max(true_positive + false_positive, 1)
        recall = true_positive / max(true_positive + false_negative, 1)
        f1 = 2 * precision * recall / max(precision + recall, 1e-12)
        f1_values.append(f1)
        per_class[class_name] = {
            "precision": round(precision, 6),
            "recall": round(recall, 6),
            "f1": round(f1, 6),
            "support": int(matrix[class_id, :].sum()),
        }

    return {
        "accuracy": round(float(np.mean(actual == predicted)), 6),
        "macro_f1": round(float(np.mean(f1_values)), 6),
        "per_class": per_class,
    }, matrix


def build_model(feature_count: int) -> tf.keras.Model:
    inputs = tf.keras.Input(shape=(feature_count,), name="normalized_sensor_features")
    values = inputs
    values = tf.keras.layers.Dense(
        16,
        activation="relu",
        kernel_regularizer=tf.keras.regularizers.l2(1e-4),
        name="dense_16",
    )(values)
    values = tf.keras.layers.Dense(
        8,
        activation="relu",
        kernel_regularizer=tf.keras.regularizers.l2(1e-4),
        name="dense_8",
    )(values)
    outputs = tf.keras.layers.Dense(3, activation="softmax", name="occupancy")(values)
    model = tf.keras.Model(inputs=inputs, outputs=outputs, name="occupancy_mlp")
    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=1e-3),
        loss="sparse_categorical_crossentropy",
        metrics=["accuracy"],
    )
    return model


def balanced_class_weights(labels: np.ndarray) -> dict[int, float]:
    counts = np.bincount(labels, minlength=len(CLASS_NAMES))
    total = len(labels)
    return {
        class_id: total / (len(CLASS_NAMES) * max(int(count), 1))
        for class_id, count in enumerate(counts)
    }


def convert_float_tflite(model: tf.keras.Model) -> bytes:
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    return converter.convert()


def convert_int8_tflite(
    model: tf.keras.Model,
    representative_inputs: np.ndarray,
) -> bytes:
    def representative_dataset():
        for row in representative_inputs[: min(len(representative_inputs), 300)]:
            yield [row.reshape(1, -1).astype(np.float32)]

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_dataset
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    return converter.convert()


def int8_predict(model_content: bytes, inputs: np.ndarray) -> np.ndarray:
    interpreter = tf.lite.Interpreter(model_content=model_content)
    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]
    input_scale, input_zero = input_details["quantization"]
    output_scale, output_zero = output_details["quantization"]
    probabilities: list[np.ndarray] = []
    for row in inputs:
        quantized = np.clip(
            np.round(row / input_scale + input_zero), -128, 127
        ).astype(np.int8)
        interpreter.set_tensor(input_details["index"], quantized.reshape(1, -1))
        interpreter.invoke()
        output = interpreter.get_tensor(output_details["index"])[0]
        probabilities.append((output.astype(np.float32) - output_zero) * output_scale)
    return np.vstack(probabilities)


def main() -> None:
    args = parse_args()
    tf.keras.utils.set_random_seed(args.seed)
    try:
        tf.config.experimental.enable_op_determinism()
    except Exception:
        pass

    feature_names = FEATURE_SETS[args.feature_set]
    data = assign_recording_splits(pd.read_csv(args.windows), args.split_config)
    splits = {
        split: data.loc[data["split"].eq(split)].copy()
        for split in ["train", "validation", "test"]
    }
    arrays = {
        split: (
            frame[feature_names].to_numpy(dtype=np.float32),
            frame["class_id"].to_numpy(dtype=np.int32),
        )
        for split, frame in splits.items()
    }

    feature_mean = arrays["train"][0].mean(axis=0)
    feature_std = arrays["train"][0].std(axis=0)
    feature_std[feature_std < 1e-6] = 1.0
    normalized_arrays = {
        split: (
            ((inputs - feature_mean) / feature_std).astype(np.float32),
            labels,
        )
        for split, (inputs, labels) in arrays.items()
    }

    model = build_model(len(feature_names))
    callbacks = [
        tf.keras.callbacks.EarlyStopping(
            monitor="val_loss",
            patience=25,
            restore_best_weights=True,
        ),
        tf.keras.callbacks.ReduceLROnPlateau(
            monitor="val_loss",
            patience=10,
            factor=0.5,
            min_lr=1e-5,
        ),
    ]
    history = model.fit(
        normalized_arrays["train"][0],
        normalized_arrays["train"][1],
        validation_data=normalized_arrays["validation"],
        epochs=args.epochs,
        batch_size=args.batch_size,
        class_weight=balanced_class_weights(normalized_arrays["train"][1]),
        callbacks=callbacks,
        verbose=0,
    )

    args.artifacts.mkdir(parents=True, exist_ok=True)
    model.save(args.artifacts / "occupancy_model.keras")
    float_tflite = convert_float_tflite(model)
    int8_tflite = convert_int8_tflite(model, normalized_arrays["train"][0])
    (args.artifacts / "occupancy_model_float.tflite").write_bytes(float_tflite)
    (args.artifacts / "occupancy_model_int8.tflite").write_bytes(int8_tflite)

    metrics: dict[str, object] = {}
    for split, (inputs, labels) in normalized_arrays.items():
        probabilities = model.predict(inputs, verbose=0)
        predicted = probabilities.argmax(axis=1)
        split_metrics, matrix = classification_metrics(labels, predicted)
        metrics[split] = split_metrics
        pd.DataFrame(
            matrix,
            index=[f"actual_{name}" for name in CLASS_NAMES],
            columns=[f"predicted_{name}" for name in CLASS_NAMES],
        ).to_csv(args.artifacts / f"confusion_matrix_{split}.csv")
        if split == "test":
            predictions = splits[split][
                ["window_id", "recording_id", "occupancy_label"]
            ].copy()
            predictions["predicted_class"] = [CLASS_NAMES[index] for index in predicted]
            for class_id, class_name in enumerate(CLASS_NAMES):
                predictions[f"probability_{class_name.lower()}"] = probabilities[:, class_id]
            predictions.to_csv(args.artifacts / "test_predictions.csv", index=False)

    int8_probabilities = int8_predict(int8_tflite, normalized_arrays["test"][0])
    int8_metrics, int8_matrix = classification_metrics(
        normalized_arrays["test"][1], int8_probabilities.argmax(axis=1)
    )
    metrics["test_int8"] = int8_metrics
    pd.DataFrame(
        int8_matrix,
        index=[f"actual_{name}" for name in CLASS_NAMES],
        columns=[f"predicted_{name}" for name in CLASS_NAMES],
    ).to_csv(args.artifacts / "confusion_matrix_test_int8.csv")

    pd.DataFrame(history.history).to_csv(
        args.artifacts / "training_history.csv", index=False
    )
    split_summary = (
        data.groupby(["split", "occupancy_label"], as_index=False)
        .agg(windows=("window_id", "size"), recordings=("recording_id", "nunique"))
    )
    split_summary.to_csv(args.artifacts / "split_summary.csv", index=False)

    output = {
        "source_windows": str(args.windows.resolve()),
        "split_config": (
            str(args.split_config.resolve()) if args.split_config is not None else None
        ),
        "feature_set": args.feature_set,
        "feature_names": feature_names,
        "epochs_completed": len(history.history["loss"]),
        "model_sizes_bytes": {
            "keras": int((args.artifacts / "occupancy_model.keras").stat().st_size),
            "float_tflite": len(float_tflite),
            "int8_tflite": len(int8_tflite),
        },
        "split_method": (
            "explicit complete-recording split configuration"
            if args.split_config is not None
            else "complete recording sessions held out for validation and test"
        ),
        "metrics": metrics,
    }
    (args.artifacts / "metrics.json").write_text(
        json.dumps(output, indent=2), encoding="utf-8"
    )
    (args.artifacts / "feature_config.json").write_text(
        json.dumps(
            {
                "feature_names": feature_names,
                "class_names": CLASS_NAMES,
                "window_samples": 30,
                "step_samples": 5,
                "normalization_mean": feature_mean.tolist(),
                "normalization_standard_deviation": feature_std.tolist(),
                "model_input": "(feature - mean) / standard_deviation",
            },
            indent=2,
        ),
        encoding="utf-8",
    )
    print(json.dumps(output, indent=2))


if __name__ == "__main__":
    main()
