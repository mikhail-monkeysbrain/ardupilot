#!/usr/bin/env python3
"""Generate persistent ArduPilot VTX parameters from Betaflight JSON/CLI.

Two-phase import: import staging parameters first, verify readback, then import\nthe separate commit-marker file. Reboot after the second import.\nNever assume Mission Planner preserves row ordering or duplicate parameter names.
Frequency-only in this version: custom power codes are explicitly rejected.
"""
import argparse
import json
from pathlib import Path
from vtx_table_normalize import normalize_cli, normalize_json


def checksum(table):
    h = 2166136261
    h = ((h ^ len(table["bands"])) * 16777619) & 0xffffffff
    for b in range(11):
        freqs = table["bands"][b]["frequencies"] if b < len(table["bands"]) else []
        for c in range(8):
            f = freqs[c] if c < len(freqs) else 0
            for byte in (f & 255, f >> 8):
                h = ((h ^ byte) * 16777619) & 0xffffffff
    return (h & 0xffffff) | 1


def parameters(table):
    if table["powerlevels"]:
        raise ValueError("custom power codes cannot yet be stored by AP_Param VTX table")
    result = []
    for b in range(11):
        freqs = table["bands"][b]["frequencies"] if b < len(table["bands"]) else []
        for c in range(8):
            result.append((f"VTX_TB{b}_C{c + 1}", freqs[c] if c < len(freqs) else 0))
    result.append(("VTX_TCNT", len(table["bands"])))
    # CRC is intentionally emitted in a separate file, never in this list.
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("-o", "--output", required=True, type=Path)
    args = parser.parse_args()
    raw = args.source.read_text(encoding="utf-8-sig")
    table = normalize_json(json.loads(raw)) if args.source.suffix.lower() == ".json" else normalize_cli(raw)
    lines = [f"{name},{value}" for name, value in parameters(table)]
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {len(lines)} parameter assignments to {args.output}")


if __name__ == "__main__":
    main()
