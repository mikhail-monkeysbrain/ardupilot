#!/usr/bin/env python3
"""Compile Betaflight/JTZero JSON or CLI into a versioned VTX table binary.

Offline only: this does NOT upload to a flight controller. The target firmware
does not yet implement the receiving @VTX backend. No microSD is required by
the proposed protocol.

Binary format, little endian:
  4s magic JTV1, uint16 version=1, uint16 length,
  uint8 bands, uint8 channels=8, uint8 power_count, uint8 reserved,
  11*8 uint16 frequencies (zero = unavailable),
  10*(uint16 protocol_code, uint16 mw_or_zero) power records,
  uint32 crc32 of all preceding bytes.
All unused entries zeroed. No implicit mapping of codes to milliwatts.
"""
import argparse
import json
import struct
import zlib
from pathlib import Path
from vtx_table_normalize import normalize_json, normalize_cli

BANDS = 11
CHANNELS = 8
POWERS = 10
HEADER = "<4sHHBBBB"
TOTAL = struct.calcsize(HEADER) + BANDS * CHANNELS * 2 + POWERS * 4 + 4


def pack_table(table):
    bands = table["bands"]
    powers = table["powerlevels"]
    if not 1 <= len(bands) <= BANDS or len(powers) > POWERS:
        raise ValueError("table exceeds binary format limits")
    freqs = [0] * (BANDS * CHANNELS)
    for i, band in enumerate(bands):
        for j, freq in enumerate(band["frequencies"]):
            freqs[i * CHANNELS + j] = freq
    power_values = []
    for p in powers:
        # A protocol power code alone is not a physical RF power measurement.
        power_values.extend((p["value"], 0))
    power_values += [0] * (POWERS * 2 - len(power_values))
    raw = (struct.pack(HEADER, b"JTV1", 1, TOTAL, len(bands), CHANNELS, len(powers), 0)
           + struct.pack("<" + "H" * len(freqs), *freqs)
           + struct.pack("<" + "H" * len(power_values), *power_values))
    return raw + struct.pack("<I", zlib.crc32(raw) & 0xffffffff)


def unpack_table(data):
    if len(data) != TOTAL:
        raise ValueError(f"expected {TOTAL} bytes, got {len(data)}")
    if zlib.crc32(data[:-4]) & 0xffffffff != struct.unpack_from("<I", data, TOTAL - 4)[0]:
        raise ValueError("CRC32 mismatch")
    magic, version, size, bands, channels, powers, reserved = struct.unpack_from(HEADER, data)
    if (magic, version, size, channels, reserved) != (b"JTV1", 1, TOTAL, CHANNELS, 0):
        raise ValueError("invalid header")
    if not 1 <= bands <= BANDS or powers > POWERS:
        raise ValueError("invalid counts")
    pos = struct.calcsize(HEADER)
    freqs = struct.unpack_from("<" + "H" * (BANDS * CHANNELS), data, pos)
    pos += BANDS * CHANNELS * 2
    power_records = [struct.unpack_from("<HH", data, pos + i * 4) for i in range(POWERS)]
    if any(freq != 0 and not 100 <= freq <= 65535 for freq in freqs):
        raise ValueError("invalid frequency")
    if any(freqs[b * CHANNELS + c] for b in range(bands, BANDS) for c in range(CHANNELS)):
        raise ValueError("unused band has nonzero frequency")
    if any(power_records[i] != (0, 0) for i in range(powers, POWERS)):
        raise ValueError("unused power entry is nonzero")
    return {"bands": bands, "channels": channels, "powerlevels": powers,
            "frequencies": [list(freqs[i * CHANNELS:(i + 1) * CHANNELS]) for i in range(bands)],
            "power_codes": [power_records[i][0] for i in range(powers)]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("-o", "--output", required=True, type=Path)
    args = parser.parse_args()
    try:
        raw = args.source.read_text(encoding="utf-8-sig")
        table = normalize_json(json.loads(raw)) if args.source.suffix.lower() == ".json" else normalize_cli(raw)
        payload = pack_table(table)
        unpack_table(payload)
        args.output.write_bytes(payload)
    except (OSError, ValueError, KeyError, TypeError, struct.error) as exc:
        parser.error(str(exc))
    print(f"Packed {len(table['bands'])} bands, {len(table['powerlevels'])} power codes into {len(payload)} bytes: {args.output}")


if __name__ == "__main__":
    main()
