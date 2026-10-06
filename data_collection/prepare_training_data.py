"""Clean one or more raw logger CSV files for the full-sensor AI pipeline."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import pandas as pd


RAW_REQUIRED = [
    "timestamp",
    "run_id",
    "sequence",
    "mcu_time_ms",
    "session_id",
    "pir_1",
    "pir_2",
    "tof_mm",
    "tof_status",
    "temperature_c",
    "humidity_percent",
    "pressure_hpa",
    "light_lux",
    "sample_valid",
    "occupancy_label",
]
TRAINING_COLUMNS = [
    "timestamp",
    "recording_id",
    "session_id",
    "segment_id",
    "sequence",
    "occupant_count",
    "ac_on",
    "light_on",
    "pir_1",
    "pir_2",
    "tof_mm",
    "tof_valid",
    "tof_status",
    "temperature_c",
    "humidity_percent",
    "pressure_hpa",
    "light_lux",
    "occupancy_label",
]
NUMERIC_COLUMNS = [
    "sequence",
    "mcu_time_ms",
    "pir_1",
    "pir_2",
    "tof_mm",
    "tof_status",
    "temperature_c",
    "humidity_percent",
    "pressure_hpa",
    "light_lux",
    "sample_valid",
]
LABELS = ["EMPTY", "LOW", "HIGH"]
LABEL_TO_COUNT = {"EMPTY": 0, "LOW": 1, "HIGH": 2}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="+", type=Path, help="Raw CSV file(s)")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).resolve().parent
        / "collected_data"
        / "processed"
        / "training_dataset.csv",
    )
    parser.add_argument(
        "--max-tof-gap",
        type=int,
        default=0,
        help="Causally forward-fill at most this many invalid ToF samples per session",
    )
    parser.add_argument(
        "--trim-seconds",
        type=float,
        default=5.0,
        help="Remove this many seconds from both the start and end of every session",
    )
    parser.add_argument(
        "--trim-overrides",
        type=Path,
        help=(
            "Optional CSV with recording_id, trim_start_rows, and "
            "trim_end_rows for sessions that need exact row-based edge trimming"
        ),
    )
    parser.add_argument(
        "--gap-threshold-seconds",
        type=float,
        default=5.0,
        help="Start a new feature segment after a larger timestamp gap",
    )
    parser.add_argument(
        "--invalid-tof-value",
        type=float,
        default=2000.0,
        help="Distance value used when VL53L0X reports no valid target",
    )
    parser.add_argument(
        "--median-window",
        type=int,
        default=1,
        help=(
            "Optional causal rolling-median window for continuous sensors; "
            "1 keeps the accepted readings unchanged"
        ),
    )
    return parser.parse_args()


def load_trim_overrides(path: Path | None) -> dict[str, tuple[int, int]]:
    if path is None:
        return {}
    if not path.exists():
        raise FileNotFoundError(path)
    frame = pd.read_csv(path)
    required = ["recording_id", "trim_start_rows", "trim_end_rows"]
    missing = [column for column in required if column not in frame.columns]
    if missing:
        raise ValueError(f"{path}: missing columns {missing}")
    if frame["recording_id"].duplicated().any():
        raise ValueError(f"{path}: recording_id values must be unique")
    for column in ["trim_start_rows", "trim_end_rows"]:
        frame[column] = pd.to_numeric(frame[column], errors="coerce")
        invalid = (
            frame[column].isna()
            | (frame[column] < 0)
            | (frame[column] % 1 != 0)
        )
        if invalid.any():
            raise ValueError(f"{path}: {column} must contain non-negative integers")
    return {
        str(row.recording_id).strip(): (
            int(row.trim_start_rows),
            int(row.trim_end_rows),
        )
        for row in frame[required].itertuples(index=False)
    }


def load_raw_files(paths: list[Path]) -> pd.DataFrame:
    frames: list[pd.DataFrame] = []
    for path in paths:
        if not path.exists():
            raise FileNotFoundError(path)
        first_field = path.read_text(encoding="utf-8-sig").splitlines()[0].split(",")[0]
        if first_field.strip() == "timestamp":
            frame = pd.read_csv(path)
        else:
            frame = pd.read_csv(path, header=None, names=RAW_REQUIRED)
        missing = [column for column in RAW_REQUIRED if column not in frame.columns]
        if missing:
            raise ValueError(f"{path}: missing columns {missing}")
        frame = frame[RAW_REQUIRED].copy()
        frame["source_file"] = path.stem
        frames.append(frame)
    if not frames:
        raise ValueError("No input files supplied")
    return pd.concat(frames, ignore_index=True)


def append_reason(reason: pd.Series, mask: pd.Series, message: str) -> pd.Series:
    reason.loc[mask] = reason.loc[mask].apply(
        lambda old: message if not old else f"{old};{message}"
    )
    return reason


def clean_data(
    raw: pd.DataFrame,
    max_tof_gap: int,
    median_window: int = 1,
    trim_seconds: float = 5.0,
    trim_overrides: dict[str, tuple[int, int]] | None = None,
    gap_threshold_seconds: float = 5.0,
    invalid_tof_value: float = 2000.0,
) -> tuple[pd.DataFrame, pd.DataFrame, dict[str, object]]:
    data = raw.copy()
    source_rows = len(data)
    data["timestamp"] = pd.to_datetime(data["timestamp"], errors="coerce", utc=True)
    for column in NUMERIC_COLUMNS:
        data[column] = pd.to_numeric(data[column], errors="coerce")
    data["occupancy_label"] = (
        data["occupancy_label"].astype(str).str.strip().str.upper()
    )
    data["run_id"] = data["run_id"].astype(str).str.strip()
    data["original_session_id"] = data["session_id"].astype(str).str.strip()
    data["recording_id"] = data["source_file"].astype(str)
    data["session_id"] = (
        data["source_file"].astype(str)
        + "__"
        + data["run_id"]
        + "__"
        + data["original_session_id"]
    )
    data = data.sort_values(
        ["source_file", "run_id", "original_session_id", "sequence"],
        kind="stable",
    ).reset_index(drop=True)

    data["occupant_count"] = data["occupancy_label"].map(LABEL_TO_COUNT)
    data["ac_on"] = data["source_file"].str.contains("_ACON_", regex=False).astype(int)
    data["light_on"] = (
        data["source_file"].str.contains("_LIGHTON", regex=False).astype(int)
    )

    group_columns = ["source_file", "run_id", "original_session_id"]
    grouped = data.groupby(group_columns, sort=False)
    first_in_group = grouped.cumcount().eq(0)
    sequence_gap = grouped["sequence"].diff().ne(1)
    time_gap = grouped["mcu_time_ms"].diff().gt(gap_threshold_seconds * 1000.0)
    gap_break = (~first_in_group) & (sequence_gap | time_gap)
    data["segment_number"] = (
        gap_break.groupby([data[column] for column in group_columns]).cumsum() + 1
    )
    data["segment_id"] = (
        data["session_id"]
        + "__SEG_"
        + data["segment_number"].astype(int).astype(str).str.zfill(2)
    )

    first_mcu_time = grouped["mcu_time_ms"].transform("min")
    last_mcu_time = grouped["mcu_time_ms"].transform("max")
    from_start_seconds = (data["mcu_time_ms"] - first_mcu_time) / 1000.0
    to_end_seconds = (last_mcu_time - data["mcu_time_ms"]) / 1000.0
    trim_overrides = trim_overrides or {}
    edge_trim_mask = (from_start_seconds < trim_seconds) | (
        to_end_seconds < trim_seconds
    )
    row_from_start = grouped.cumcount()
    row_to_end = grouped["sequence"].transform("size") - row_from_start - 1
    for recording_id, (start_rows, end_rows) in trim_overrides.items():
        recording_mask = data["source_file"].eq(recording_id)
        edge_trim_mask.loc[recording_mask] = (
            (row_from_start.loc[recording_mask] < start_rows)
            | (row_to_end.loc[recording_mask] < end_rows)
        )

    reason = pd.Series("", index=data.index, dtype="object")
    reason = append_reason(reason, edge_trim_mask, "edge_trim")
    reason = append_reason(reason, data["timestamp"].isna(), "invalid_timestamp")
    reason = append_reason(
        reason,
        data[NUMERIC_COLUMNS].isna().any(axis=1),
        "missing_or_non_numeric",
    )
    reason = append_reason(
        reason, ~data["occupancy_label"].isin(LABELS), "invalid_label"
    )
    reason = append_reason(reason, ~data["pir_1"].isin([0, 1]), "invalid_pir_1")
    reason = append_reason(reason, ~data["pir_2"].isin([0, 1]), "invalid_pir_2")
    reason = append_reason(
        reason,
        ~data["temperature_c"].between(-40.0, 85.0),
        "temperature_out_of_range",
    )
    reason = append_reason(
        reason,
        ~data["humidity_percent"].between(0.0, 100.0),
        "humidity_out_of_range",
    )
    reason = append_reason(
        reason,
        ~data["pressure_hpa"].between(300.0, 1100.0),
        "pressure_out_of_range",
    )
    reason = append_reason(
        reason, ~data["light_lux"].between(0.0, 65535.0), "light_out_of_range"
    )
    reason = append_reason(reason, data["sample_valid"] != 1, "sensor_read_failed")

    duplicate_mask = data.duplicated(["session_id", "timestamp"], keep="first")
    reason = append_reason(reason, duplicate_mask, "duplicate_session_timestamp")

    label_count = data.groupby("session_id")["occupancy_label"].transform("nunique")
    reason = append_reason(reason, label_count > 1, "mixed_labels_in_session")

    tof_original_valid = (data["tof_status"] == 0) & data["tof_mm"].between(20.0, 2000.0)
    data["tof_mm_raw"] = data["tof_mm"]
    data["tof_valid"] = tof_original_valid.astype(int)
    tof_clean = data["tof_mm"].where(tof_original_valid)
    if max_tof_gap > 0:
        tof_clean = tof_clean.groupby(data["segment_id"], sort=False).ffill(
            limit=max_tof_gap
        )
    data["tof_imputed"] = (~tof_original_valid) & tof_clean.notna()
    data["tof_mm"] = tof_clean.fillna(invalid_tof_value)

    accepted_mask = reason.eq("")
    accepted = data.loc[accepted_mask].copy()
    rejected = data.loc[~accepted_mask].copy()
    rejected["rejection_reason"] = reason.loc[~accepted_mask]

    accepted = accepted.sort_values(["session_id", "timestamp"], kind="stable")
    if median_window > 1:
        filter_columns = [
            "tof_mm",
            "temperature_c",
            "humidity_percent",
            "light_lux",
        ]
        for column in filter_columns:
            accepted[column] = accepted.groupby("segment_id", sort=False)[column].transform(
                lambda values: values.rolling(
                    window=median_window,
                    min_periods=1,
                ).median()
            )
    training = accepted[TRAINING_COLUMNS].copy()
    training["timestamp"] = training["timestamp"].dt.strftime("%Y-%m-%dT%H:%M:%S.%fZ")
    training["pir_1"] = training["pir_1"].astype(int)
    training["pir_2"] = training["pir_2"].astype(int)
    training["tof_valid"] = training["tof_valid"].astype(int)
    training["tof_status"] = training["tof_status"].astype(int)
    training["sequence"] = training["sequence"].astype(int)
    training["occupant_count"] = training["occupant_count"].astype(int)
    training["ac_on"] = training["ac_on"].astype(int)
    training["light_on"] = training["light_on"].astype(int)
    training["tof_mm"] = training["tof_mm"].round(1)

    sessions = (
        training.groupby(["occupancy_label", "session_id"], as_index=False)
        .agg(records=("timestamp", "size"))
        if not training.empty
        else pd.DataFrame(columns=["occupancy_label", "session_id", "records"])
    )
    records_per_class = (
        training["occupancy_label"].value_counts().reindex(LABELS, fill_value=0)
    )
    sessions_per_class = (
        sessions["occupancy_label"].value_counts().reindex(LABELS, fill_value=0)
    )
    warnings: list[str] = []
    for label in LABELS:
        if int(sessions_per_class[label]) < 3:
            warnings.append(f"{label} has fewer than 3 independent sessions")
    short_sessions = sessions.loc[sessions["records"] < 300, "session_id"].tolist()
    if short_sessions:
        warnings.append(
            f"{len(short_sessions)} session(s) have fewer than 300 accepted rows"
        )

    report: dict[str, object] = {
        "source_rows": int(source_rows),
        "accepted_rows": int(len(training)),
        "rejected_rows": int(len(rejected)),
        "edge_trim_seconds_per_side": float(trim_seconds),
        "edge_trim_overrides": {
            recording_id: {
                "start_rows": int(values[0]),
                "end_rows": int(values[1]),
            }
            for recording_id, values in sorted(trim_overrides.items())
        },
        "edge_trimmed_rows": int(edge_trim_mask.sum()),
        "gap_threshold_seconds": float(gap_threshold_seconds),
        "gap_breaks_detected": int(gap_break.sum()),
        "tof_original_valid_rows": int(tof_original_valid.sum()),
        "tof_no_target_encoded_rows": int((~tof_original_valid).sum()),
        "invalid_tof_encoded_value_mm": float(invalid_tof_value),
        "tof_rows_forward_filled": int(data["tof_imputed"].sum()),
        "max_tof_gap_samples": int(max_tof_gap),
        "causal_median_window_samples": int(median_window),
        "records_per_class": {label: int(records_per_class[label]) for label in LABELS},
        "sessions_per_class": {label: int(sessions_per_class[label]) for label in LABELS},
        "warnings": warnings,
    }
    return training, rejected, report


def main() -> None:
    args = parse_args()
    if args.max_tof_gap < 0:
        raise SystemExit("--max-tof-gap must be zero or greater")
    if args.trim_seconds < 0:
        raise SystemExit("--trim-seconds must be zero or greater")
    if args.gap_threshold_seconds <= 0:
        raise SystemExit("--gap-threshold-seconds must be greater than zero")
    if args.invalid_tof_value < 20.0 or args.invalid_tof_value > 2000.0:
        raise SystemExit("--invalid-tof-value must be between 20 and 2000 mm")
    if args.median_window < 1 or args.median_window % 2 == 0:
        raise SystemExit("--median-window must be a positive odd number")
    try:
        raw = load_raw_files(args.inputs)
        trim_overrides = load_trim_overrides(args.trim_overrides)
        training, rejected, report = clean_data(
            raw,
            args.max_tof_gap,
            args.median_window,
            args.trim_seconds,
            trim_overrides,
            args.gap_threshold_seconds,
            args.invalid_tof_value,
        )
    except (FileNotFoundError, ValueError) as exc:
        raise SystemExit(str(exc)) from exc

    args.output.parent.mkdir(parents=True, exist_ok=True)
    training.to_csv(args.output, index=False)
    rejected_path = args.output.with_suffix(".rejected.csv")
    report_path = args.output.with_suffix(".report.json")
    rejected.to_csv(rejected_path, index=False)
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(json.dumps(report, indent=2))
    print(f"Training CSV: {args.output.resolve()}")
    print(f"Rejected rows: {rejected_path.resolve()}")
    print(f"Cleaning report: {report_path.resolve()}")


if __name__ == "__main__":
    main()
