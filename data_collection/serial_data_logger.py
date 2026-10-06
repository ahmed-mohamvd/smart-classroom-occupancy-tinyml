"""Save STM32 sensor rows from USB serial or an HC-06 Bluetooth COM port.

The firmware emits records beginning with ``DATA,``.  Diagnostic and EVENT
lines are shown on screen but are not mixed into the CSV dataset.
"""

from __future__ import annotations

import argparse
import csv
import queue
import sys
import threading
from datetime import datetime, timedelta
from pathlib import Path
from typing import Iterable, TextIO


FIRMWARE_COLUMNS = [
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
OUTPUT_COLUMNS = ["timestamp", "run_id", *FIRMWARE_COLUMNS]
COMMANDS = {"E", "L", "H", "S", "X", "?", "Q"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--port", help="Serial/Bluetooth COM port, for example COM8")
    source.add_argument(
        "--input-file",
        type=Path,
        help="Replay a saved serial-text file instead of opening a COM port",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=9600,
        help="HC-06 default is 9600; use 115200 for ST-LINK USB serial",
    )
    parser.add_argument("--output", type=Path, help="Destination raw CSV file")
    return parser.parse_args()


def make_run_id() -> str:
    return datetime.now().astimezone().strftime("RUN_%Y%m%d_%H%M%S")


def default_output_path(run_id: str) -> Path:
    return Path(__file__).resolve().parent / "collected_data" / "raw" / f"{run_id}.csv"


def parse_data_line(line: str) -> list[str] | None:
    try:
        fields = next(csv.reader([line]))
    except csv.Error:
        return None
    if not fields or fields[0].strip().upper() != "DATA":
        return None
    values = [value.strip() for value in fields[1:]]
    if len(values) != len(FIRMWARE_COLUMNS):
        return None
    if values[-1].upper() not in {"EMPTY", "LOW", "HIGH"}:
        return None
    try:
        int(values[0])
        int(values[1])
        int(values[3])
        int(values[4])
        int(values[5])
        int(values[6])
        float(values[7])
        float(values[8])
        float(values[9])
        float(values[10])
        int(values[11])
    except ValueError:
        return None
    values[-1] = values[-1].upper()
    return values


def keyboard_commands(command_queue: queue.Queue[str]) -> None:
    while True:
        try:
            command = input().strip().upper()
        except EOFError:
            return
        if command in COMMANDS:
            command_queue.put(command)
        elif command:
            print("Unknown command. Use E, L, H, S, X, ?, or Q.")


def iter_text_lines(stream: TextIO) -> Iterable[str]:
    for line in stream:
        yield line.rstrip("\r\n")


def capture_lines(
    lines: Iterable[str],
    output_path: Path,
    run_id: str,
    command_writer=None,
) -> tuple[int, int]:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    accepted = 0
    rejected = 0
    time_anchor: datetime | None = None
    mcu_anchor_ms: int | None = None
    previous_mcu_ms: int | None = None
    session_active = False
    command_queue: queue.Queue[str] = queue.Queue()

    if command_writer is not None:
        threading.Thread(
            target=keyboard_commands,
            args=(command_queue,),
            daemon=True,
        ).start()
        print(
            "Commands: E=EMPTY, L=LOW, H=HIGH, S=start, X=stop, "
            "?=status, Q=close logger"
        )
        print("Stop the STM32 session before using Q.")

    with output_path.open("w", newline="", encoding="utf-8") as output_file:
        writer = csv.writer(output_file)
        writer.writerow(OUTPUT_COLUMNS)
        output_file.flush()

        for line in lines:
            while command_writer is not None:
                try:
                    command = command_queue.get_nowait()
                except queue.Empty:
                    break
                if command == "Q":
                    if session_active:
                        print("Session is still active. Press Button 2 to stop it first.")
                        continue
                    print("Closing logger safely...")
                    return accepted, rejected
                command_writer((command + "\n").encode("ascii"))
                print(f"> sent {command}")

            if not line:
                continue

            values = parse_data_line(line)
            if values is not None:
                mcu_time_ms = int(values[1])
                if (
                    time_anchor is None
                    or mcu_anchor_ms is None
                    or (previous_mcu_ms is not None and mcu_time_ms < previous_mcu_ms)
                ):
                    time_anchor = datetime.now().astimezone()
                    mcu_anchor_ms = mcu_time_ms
                sample_time = time_anchor + timedelta(
                    milliseconds=mcu_time_ms - mcu_anchor_ms
                )
                previous_mcu_ms = mcu_time_ms
                timestamp = sample_time.isoformat(timespec="milliseconds")
                writer.writerow([timestamp, run_id, *values])
                output_file.flush()
                accepted += 1
                print(
                    f"saved {accepted:5d} | {values[2]} | {values[-1]:5s} | "
                    f"ToF={values[5]} mm S{values[6]} | Lux={values[10]}"
                )
            elif line.startswith("DATA,"):
                rejected += 1
                print(f"REJECTED malformed DATA line: {line}", file=sys.stderr)
            else:
                print(f"STM32: {line}")
                if line.startswith("EVENT,START,"):
                    session_active = True
                elif line.startswith("EVENT,STOP,"):
                    session_active = False

    return accepted, rejected


def main() -> None:
    args = parse_args()
    run_id = make_run_id()
    output_path = args.output or default_output_path(run_id)

    try:
        if args.input_file is not None:
            with args.input_file.open("r", encoding="utf-8", errors="replace") as stream:
                accepted, rejected = capture_lines(
                    iter_text_lines(stream), output_path, run_id
                )
        else:
            try:
                import serial  # type: ignore
            except ImportError as exc:
                raise SystemExit(
                    "pyserial is required for a COM port. Run: pip install pyserial"
                ) from exc

            with serial.Serial(args.port, args.baud, timeout=0.25) as port:
                print(f"Connected to {args.port} at {args.baud} baud")

                def serial_lines() -> Iterable[str]:
                    while True:
                        raw = port.readline()
                        if raw:
                            yield raw.decode("utf-8", errors="replace").strip()
                        else:
                            yield ""

                accepted, rejected = capture_lines(
                    serial_lines(), output_path, run_id, port.write
                )
    except KeyboardInterrupt:
        print("\nLogger stopped by user.")
        return

    print(f"Saved {accepted} rows to {output_path.resolve()}")
    if rejected:
        print(f"Rejected malformed DATA rows: {rejected}")


if __name__ == "__main__":
    main()
