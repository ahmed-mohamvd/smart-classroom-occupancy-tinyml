"""Convert synchronized one-second sensor readings into fixed time-window features."""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import pandas as pd


LABEL_TO_ID = {"EMPTY": 0, "LOW": 1, "HIGH": 2}

PRESENCE_FEATURES = [
    "pir_1_ratio",
    "pir_2_ratio",
    "pir_any_ratio",
    "pir_both_ratio",
    "pir_1_rising_edges",
    "pir_2_rising_edges",
    "tof_valid_ratio",
    "tof_min_mm",
    "tof_median_mm",
    "tof_std_mm",
]

ENVIRONMENT_FEATURES = [
    "temperature_mean",
    "temperature_delta",
    "humidity_mean",
    "humidity_delta",
    "pressure_mean",
    "pressure_delta",
    "light_mean",
    "light_delta",
    "light_std",
]

FEATURES = PRESENCE_FEATURES + ENVIRONMENT_FEATURES


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dataset", type=Path, help="Cleaned synchronized CSV")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--window-size",
        type=int,
        default=30,
        help="Number of approximately one-second readings in each window",
    )
    parser.add_argument(
        "--step-size",
        type=int,
        default=5,
        help="Number of readings between consecutive window starts",
    )
    return parser.parse_args()


def rising_edges(values: pd.Series) -> int:
    numeric = values.to_numpy(dtype=np.int8)
    if len(numeric) < 2:
        return 0
    return int(np.sum((numeric[1:] == 1) & (numeric[:-1] == 0)))


def window_features(window: pd.DataFrame) -> dict[str, float]:
    pir_1 = window["pir_1"].astype(int)
    pir_2 = window["pir_2"].astype(int)
    pir_any = (pir_1.eq(1) | pir_2.eq(1)).astype(int)
    pir_both = (pir_1.eq(1) & pir_2.eq(1)).astype(int)

    valid_tof = window.loc[window["tof_valid"].eq(1), "tof_mm"].astype(float)
    if valid_tof.empty:
        tof_min = 2000.0
        tof_median = 2000.0
        tof_std = 0.0
    else:
        tof_min = float(valid_tof.min())
        tof_median = float(valid_tof.median())
        tof_std = float(valid_tof.std(ddof=0))

    return {
        "pir_1_ratio": float(pir_1.mean()),
        "pir_2_ratio": float(pir_2.mean()),
        "pir_any_ratio": float(pir_any.mean()),
        "pir_both_ratio": float(pir_both.mean()),
        "pir_1_rising_edges": float(rising_edges(pir_1)),
        "pir_2_rising_edges": float(rising_edges(pir_2)),
        "tof_valid_ratio": float(window["tof_valid"].mean()),
        "tof_min_mm": tof_min,
        "tof_median_mm": tof_median,
        "tof_std_mm": tof_std,
        "temperature_mean": float(window["temperature_c"].mean()),
        "temperature_delta": float(
            window["temperature_c"].iloc[-1] - window["temperature_c"].iloc[0]
        ),
        "humidity_mean": float(window["humidity_percent"].mean()),
        "humidity_delta": float(
            window["humidity_percent"].iloc[-1]
            - window["humidity_percent"].iloc[0]
        ),
        "pressure_mean": float(window["pressure_hpa"].mean()),
        "pressure_delta": float(
            window["pressure_hpa"].iloc[-1] - window["pressure_hpa"].iloc[0]
        ),
        "light_mean": float(window["light_lux"].mean()),
        "light_delta": float(
            window["light_lux"].iloc[-1] - window["light_lux"].iloc[0]
        ),
        "light_std": float(window["light_lux"].std(ddof=0)),
    }


def build_windows(
    data: pd.DataFrame,
    window_size: int = 30,
    step_size: int = 5,
) -> pd.DataFrame:
    if window_size < 2:
        raise ValueError("window_size must be at least 2")
    if step_size < 1:
        raise ValueError("step_size must be at least 1")

    rows: list[dict[str, object]] = []
    ordered = data.sort_values(["segment_id", "timestamp"], kind="stable")
    for segment_id, segment in ordered.groupby("segment_id", sort=False):
        segment = segment.reset_index(drop=True)
        if len(segment) < window_size:
            continue
        for start in range(0, len(segment) - window_size + 1, step_size):
            window = segment.iloc[start : start + window_size]
            label = str(window["occupancy_label"].iloc[0])
            if window["occupancy_label"].nunique() != 1 or label not in LABEL_TO_ID:
                raise ValueError(f"Invalid label window in segment {segment_id}")
            row: dict[str, object] = {
                "window_id": f"{segment_id}__W_{start:05d}",
                "recording_id": str(window["recording_id"].iloc[0]),
                "session_id": str(window["session_id"].iloc[0]),
                "segment_id": str(segment_id),
                "window_start": str(window["timestamp"].iloc[0]),
                "window_end": str(window["timestamp"].iloc[-1]),
                "occupant_count": int(window["occupant_count"].iloc[0]),
                "ac_on": int(window["ac_on"].iloc[0]),
                "light_on": int(window["light_on"].iloc[0]),
                "occupancy_label": label,
                "class_id": LABEL_TO_ID[label],
            }
            row.update(window_features(window))
            rows.append(row)

    if not rows:
        raise ValueError("No complete windows could be built")
    return pd.DataFrame(rows)


def main() -> None:
    args = parse_args()
    data = pd.read_csv(args.dataset)
    windows = build_windows(data, args.window_size, args.step_size)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    windows.to_csv(args.output, index=False)
    print(f"Windows: {len(windows)}")
    print(f"Recordings: {windows['recording_id'].nunique()}")
    print(windows["occupancy_label"].value_counts().to_string())
    print(f"Output: {args.output.resolve()}")


if __name__ == "__main__":
    main()
