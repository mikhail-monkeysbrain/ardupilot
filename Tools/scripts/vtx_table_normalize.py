#!/usr/bin/env python3
"""Normalize Betaflight VTX JSON (both layouts) or CLI into canonical JSON.

This is an offline data converter, not a firmware uploader. Power values
are protocol codes, NOT necessarily milliwatts. Never infer RF power from them.
"""
import argparse
import json
from pathlib import Path


def _integer(value, field):
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError(f"{field}: integer required")
    return value


def _band(name, letter, factory, frequencies):
    if not isinstance(name, str) or not name.strip():
        raise ValueError("band name required")
    if not isinstance(letter, str) or len(letter) != 1:
        raise ValueError("band letter must be one character")
    if not isinstance(factory, bool):
        raise ValueError("factory flag must be boolean")
    if not isinstance(frequencies, list) or not 1 <= len(frequencies) <= 8:
        raise ValueError("each band requires 1..8 channels")
    for freq in frequencies:
        _integer(freq, "frequency")
        if freq != 0 and not 100 <= freq <= 32767:
            raise ValueError(f"frequency {freq} MHz outside 100..32767")
    return {"name": name, "letter": letter, "is_factory_band": factory, "frequencies": frequencies}


def normalize_json(source):
    if not isinstance(source, dict):
        raise ValueError("root must be a JSON object")
    table = source.get("vtx_table", source.get("vtxTable", source))
    if source.get("format") == "jtzero-vtx-table-v1":
        table = source
    if not isinstance(table, dict):
        raise ValueError("VTX table must be an object")
    bands = table.get("bands_list", table.get("bands"))
    if not isinstance(bands, list) or not 1 <= len(bands) <= 11:
        raise ValueError("1..11 bands required")
    out_bands = []
    for b in bands:
        if not isinstance(b, dict):
            raise ValueError("band must be an object")
        out_bands.append(_band(
            b.get("name"), b.get("letter"), b.get("is_factory_band", b.get("isFactoryBand", False)),
            b.get("frequencies")))
    powers = table.get("powerlevels_list", table.get("powerlevels", []))
    if not isinstance(powers, list) or len(powers) > 10:
        raise ValueError("powerlevels must contain at most 10 entries")
    out_powers = []
    for p in powers:
        if not isinstance(p, dict) or not isinstance(p.get("label"), str):
            raise ValueError("power entry must contain a label")
        value = _integer(p.get("value"), "power code")
        if not 0 <= value <= 65535:
            raise ValueError("power code out of range")
        out_powers.append({"value": value, "label": p["label"]})
    return {"format": "jtzero-vtx-table-v1", "bands": out_bands, "powerlevels": out_powers}


def normalize_cli(source):
    bands_count = channels_count = None
    bands = {}
    for line_no, line in enumerate(source.splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#") or line == "save":
            continue
        words = line.split()
        if words[:2] == ["vtxtable", "bands"] and len(words) == 3:
            bands_count = int(words[2])
        elif words[:2] == ["vtxtable", "channels"] and len(words) == 3:
            channels_count = int(words[2])
        elif words[:2] == ["vtxtable", "band"] and len(words) >= 7:
            index = int(words[2])
            name, letter = words[3:5]
            if words[5] not in ("FACTORY", "CUSTOM"):
                raise ValueError(f"line {line_no}: expected FACTORY or CUSTOM")
            freqs = [int(x) for x in words[6:]]
            if index in bands:
                raise ValueError(f"line {line_no}: duplicate band {index}")
            bands[index] = _band(name, letter, words[5] == "FACTORY", freqs)
        else:
            raise ValueError(f"line {line_no}: unsupported CLI command")
    if bands_count is None or channels_count is None:
        raise ValueError("CLI must declare vtxtable bands and channels")
    if not 1 <= bands_count <= 11 or not 1 <= channels_count <= 8:
        raise ValueError("band/channel count exceeds current normalization limit")
    if set(bands) != set(range(1, bands_count + 1)):
        raise ValueError("missing or extra band definition")
    if any(len(b["frequencies"]) != channels_count for b in bands.values()):
        raise ValueError("frequency count differs from vtxtable channels")
    return {"format": "jtzero-vtx-table-v1", "bands": [bands[i] for i in range(1, bands_count + 1)], "powerlevels": []}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()
    try:
        raw = args.source.read_text(encoding="utf-8-sig")
        normalized = normalize_json(json.loads(raw)) if args.source.suffix.lower() == ".json" else normalize_cli(raw)
    except (OSError, ValueError, TypeError, KeyError) as exc:
        parser.error(str(exc))
    rendered = json.dumps(normalized, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")


if __name__ == "__main__":
    main()
