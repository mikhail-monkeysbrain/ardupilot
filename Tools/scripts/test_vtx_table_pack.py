#!/usr/bin/env python3
"""Offline pack/unpack tests; no hardware access."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from vtx_table_normalize import normalize_json
from vtx_table_pack import TOTAL, pack_table, unpack_table


class VtxPackTests(unittest.TestCase):
    def setUp(self):
        self.table = normalize_json({"vtx_table": {
            "bands_list": [
                {"name": "A", "letter": "A", "is_factory_band": False,
                 "frequencies": [3700, 3720, 3740, 3760, 3780, 3800, 3820, 3840]},
                {"name": "B", "letter": "B", "is_factory_band": False,
                 "frequencies": [3860, 3880, 3900, 3920, 3940, 3960, 3980, 4000]},
                {"name": "C", "letter": "C", "is_factory_band": False,
                 "frequencies": [4020, 4040, 4060, 4080]}],
            "powerlevels_list": [{"value": i, "label": str(i)} for i in range(1, 5)]
        }})

    def test_complete_example_not_truncated(self):
        payload = pack_table(self.table)
        self.assertEqual(len(payload), TOTAL)
        decoded = unpack_table(payload)
        self.assertEqual(decoded["bands"], 3)
        self.assertEqual(decoded["frequencies"][2][:4], [4020, 4040, 4060, 4080])
        self.assertEqual(decoded["frequencies"][2][4:], [0, 0, 0, 0])
        self.assertEqual(decoded["power_codes"], [1, 2, 3, 4])

    def test_multiple_frequency_ranges_including_above_6ghz(self):
        frequencies = [1200, 3500, 4400, 4900, 5800, 6200, 7200, 10000]
        table = normalize_json({"bands": [{
            "name": "MULTI", "letter": "M", "is_factory_band": False,
            "frequencies": frequencies
        }]})
        decoded = unpack_table(pack_table(table))
        self.assertEqual(decoded["frequencies"][0], frequencies)

    def test_crc_rejects_corruption(self):
        payload = bytearray(pack_table(self.table))
        payload[20] ^= 1
        with self.assertRaisesRegex(ValueError, "CRC32"):
            unpack_table(bytes(payload))

    def test_11_bands_88_channels(self):
        band = {"name": "A", "letter": "A", "is_factory_band": False,
                "frequencies": [3330, 3350, 3370, 3390, 3410, 3430, 3450, 3470]}
        table = normalize_json({"bands": [band.copy() for _ in range(11)]})
        decoded = unpack_table(pack_table(table))
        self.assertEqual(len(decoded["frequencies"]), 11)
        self.assertEqual(sum(len(b) for b in decoded["frequencies"]), 88)


if __name__ == "__main__":
    unittest.main()
