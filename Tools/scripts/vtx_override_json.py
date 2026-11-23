#!/usr/bin/env python3
"""Convert Betaflight VTX table JSON to JTZero VTX override parameters.

This experimental firmware supports only 16 frequency overrides and retains
11 built-in bands. This tool refuses unsupported data instead of truncating.
Output is a Mission Planner-compatible PARAM,VALUE file by default.
"""
import argparse
import json
from pathlib import Path

MAX_OVERRIDES = 16
MAX_BANDS = 11
MAX_CHANNELS = 8
MIN_FREQ = 1000
MAX_FREQ = 6000

def encode(band, channel, freq):
    if not (0 <= band < MAX_BANDS and 0 <= channel < MAX_CHANNELS):
        raise ValueError("band/channel index outside firmware range")
    if not (MIN_FREQ <= freq <= MAX_FREQ):
        raise ValueError(f"frequency {freq} MHz outside {MIN_FREQ}..{MAX_FREQ}")
    word = 0x800000 | (band << 19) | (channel << 16) | freq
    return word  # exactly representable as MAVLink float32

def convert(data):
    table = data.get("vtxTable", data)
    bands = table.get("bands")
    if not isinstance(bands, list) or not bands:
        raise ValueError("vtxTable.bands must be a nonempty list")
    if len(bands) > MAX_BANDS:
        raise ValueError(f"{len(bands)} bands exceeds firmware limit {MAX_BANDS}")
    entries = []
    for bi, band in enumerate(bands):
        freqs = band.get("frequencies")
        if not isinstance(freqs, list) or len(freqs) > MAX_CHANNELS:
            raise ValueError(f"band {bi}: frequencies must have at most {MAX_CHANNELS} entries")
        for ci, freq in enumerate(freqs):
            if isinstance(freq, bool) or not isinstance(freq, int):
                raise ValueError(f"band {bi} channel {ci}: integer MHz required")
            if freq == 0:  # Betaflight uses zero for an unavailable channel
                continue
            entries.append(encode(bi, ci, freq))
    if len(entries) > MAX_OVERRIDES:
        raise ValueError(f"JSON has {len(entries)} frequencies but experimental firmware only supports {MAX_OVERRIDES}; refusing partial upload")
    powers = table.get("powerlevels", [])
    if powers:
        raise ValueError("Custom power levels are not implemented in firmware; refusing to discard JSON powerlevels")
    return {f"VTX_TBL{i + 1}": entries[i] if i < len(entries) else 0 for i in range(MAX_OVERRIDES)}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("json_file", type=Path)
    parser.add_argument("-o", "--output", type=Path, help="Write PARAM,VALUE text file")
    args = parser.parse_args()
    try:
        data = json.loads(args.json_file.read_text(encoding="utf-8-sig"))
        params = convert(data)
    except (OSError, ValueError, TypeError, KeyError) as exc:
        parser.error(str(exc))
    result = "".join(f"{name},{value}\n" for name, value in params.items())
    if args.output:
        args.output.write_text(result, encoding="utf-8")
        print(f"Wrote {len(params)} parameters to {args.output}")
    else:
        print(result, end="")

if __name__ == "__main__":
    main()
