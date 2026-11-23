#!/usr/bin/env python3
"""Generate VTX_TBL1..16 parameter values for experimental AP_VideoTX overrides.

Input CSV columns: band,channel,frequency_mhz (zero-based band/channel).
Only 16 entries are supported by the experimental firmware.
This tool generates parameters; it does not upload or activate a VTX.
"""
import argparse
import csv
from pathlib import Path

MAX_ENTRIES = 16
MAX_BANDS = 11
MAX_CHANNELS = 8

def encode(band: int, channel: int, frequency: int) -> int:
    if not 0 <= band < MAX_BANDS:
        raise ValueError(f"band must be 0..{MAX_BANDS - 1}")
    if not 0 <= channel < MAX_CHANNELS:
        raise ValueError(f"channel must be 0..{MAX_CHANNELS - 1}")
    if not 1000 <= frequency <= 6000:
        raise ValueError("frequency_mhz must be 1000..6000")
    value = 0x800000 | (band << 19) | (channel << 16) | frequency
    return value  # exact in MAVLink PARAM_SET float32 (<= 2**24)

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_file", type=Path)
    args = parser.parse_args()
    rows = []
    seen = set()
    with args.csv_file.open(newline="", encoding="utf-8-sig") as fp:
        for line, row in enumerate(csv.DictReader(fp), start=2):
            try:
                band = int(row["band"])
                channel = int(row["channel"])
                freq = int(row["frequency_mhz"])
                key = (band, channel)
                if key in seen:
                    raise ValueError(f"duplicate band/channel {key}")
                seen.add(key)
                rows.append(encode(band, channel, freq))
            except (KeyError, ValueError) as exc:
                parser.error(f"line {line}: {exc}")
    if len(rows) > MAX_ENTRIES:
        parser.error(f"maximum {MAX_ENTRIES} overrides, got {len(rows)}")
    for index in range(MAX_ENTRIES):
        print(f"VTX_TBL{index + 1},{rows[index] if index < len(rows) else 0}")

if __name__ == "__main__":
    main()
