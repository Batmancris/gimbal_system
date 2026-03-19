#!/usr/bin/env python3
"""Minimal USB CDC pitch control test sender for the STM32 firmware."""

from __future__ import annotations

import argparse
import struct
import time

import serial


HEAD0 = 0xAA
HEAD1 = 0x55
CMD_PITCH = 0x01
TAIL = 0x96
DEFAULT_PITCH_LIMIT_DEG = 20.0
DEFAULT_SWEEP_PERIOD_SEC = 30.0
DEFAULT_UPDATE_HZ = 20.0


def crc8(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x07) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


def build_pitch_frame(pitch_deg: int) -> bytes:
    payload = struct.pack("<Bh", CMD_PITCH, int(round(pitch_deg)))
    return bytes((HEAD0, HEAD1)) + payload + bytes((crc8(payload), TAIL))


def main() -> int:
    parser = argparse.ArgumentParser(description="USB CDC pitch control test sender")
    parser.add_argument("--device", default="/dev/ttyACM0", help="serial device path")
    parser.add_argument("--baudrate", type=int, default=115200, help="serial baudrate")
    parser.add_argument(
        "--period",
        type=float,
        default=DEFAULT_SWEEP_PERIOD_SEC,
        help="seconds for a single sweep from one pitch limit to the other",
    )
    parser.add_argument(
        "--limit-deg",
        type=float,
        default=DEFAULT_PITCH_LIMIT_DEG,
        help="absolute pitch limit in degrees for the smooth sweep",
    )
    parser.add_argument(
        "--update-hz",
        type=float,
        default=DEFAULT_UPDATE_HZ,
        help="frame send rate during the smooth sweep",
    )
    args = parser.parse_args()

    half_range = float(args.limit_deg)
    update_period = 1.0 / max(args.update_hz, 1.0)
    sweep_period = max(args.period, update_period)

    with serial.Serial(args.device, args.baudrate, timeout=0.1) as ser:
        start_time = time.monotonic()
        last_printed_pitch = None
        while True:
            elapsed = time.monotonic() - start_time
            phase = (elapsed % (2.0 * sweep_period)) / sweep_period

            # Triangle wave: -limit -> +limit -> -limit, which looks more like
            # a continuous stick-driven sweep than a step jump between targets.
            if phase <= 1.0:
                pitch_deg = -half_range + (2.0 * half_range * phase)
            else:
                pitch_deg = half_range - (2.0 * half_range * (phase - 1.0))

            frame = build_pitch_frame(pitch_deg)
            ser.write(frame)
            ser.flush()

            rounded_pitch = int(round(pitch_deg))
            if rounded_pitch != last_printed_pitch:
                print(f"send: pitch={rounded_pitch}")
                last_printed_pitch = rounded_pitch

            time.sleep(update_period)


if __name__ == "__main__":
    raise SystemExit(main())
