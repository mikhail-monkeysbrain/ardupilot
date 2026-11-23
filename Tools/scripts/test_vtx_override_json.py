#!/usr/bin/env python3
"""Offline validation for the experimental JSON VTX parameter converter.

Run: python3 -m unittest Tools/scripts/test_vtx_override_json.py
No flight controller or VTX is accessed.
"""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from vtx_override_json import convert, encode


class VtxJsonTests(unittest.TestCase):
    def test_exact_float32_transport(self):
        import struct
        for band in range(11):
            for channel in range(8):
                value = encode(band, channel, 5800)
                self.assertEqual(value, int(struct.unpack("<f", struct.pack("<f", value))[0]))
                self.assertEqual((value >> 19) & 0xF, band)
                self.assertEqual((value >> 16) & 0x7, channel)
                self.assertEqual(value & 0xFFFF, 5800)

    def test_single_band(self):
        data = {"vtxTable": {"bands": [{"frequencies": [5800, 5820]}]}}
        result = convert(data)
        self.assertEqual(result["VTX_TBL1"], encode(0, 0, 5800))
        self.assertEqual(result["VTX_TBL2"], encode(0, 1, 5820))
        self.assertEqual(result["VTX_TBL3"], 0)

    def test_refuse_partial_full_table(self):
        data = {"vtxTable": {"bands": [{"frequencies": [5800] * 8}] * 3}}
        with self.assertRaisesRegex(ValueError, "only supports 16"):
            convert(data)

    def test_refuse_unsupported_power_levels(self):
        data = {"vtxTable": {"bands": [{"frequencies": [5800]}], "powerlevels": [{"value": 25}]}}
        with self.assertRaisesRegex(ValueError, "power levels"):
            convert(data)

    def test_betaflight_underscore_schema(self):
        data = {"vtx_table": {"bands_list": [
            {"name": "BAND_A", "letter": "A", "is_factory_band": False,
             "frequencies": [3700, 3720]}]}}
        params = convert(data)
        self.assertEqual(params["VTX_TBL1"], encode(0, 0, 3700))
        self.assertEqual(params["VTX_TBL2"], encode(0, 1, 3720))

    def test_canonical_schema(self):
        data = {"format": "jtzero-vtx-table-v1", "bands": [
            {"name": "BAND_A", "letter": "A", "is_factory_band": False,
             "frequencies": [3700]}], "powerlevels": []}
        self.assertEqual(convert(data)["VTX_TBL1"], encode(0, 0, 3700))

    def test_reject_invalid_frequency(self):
        data = {"vtxTable": {"bands": [{"frequencies": [999]}]}}
        with self.assertRaises(ValueError):
            convert(data)


if __name__ == "__main__":
    unittest.main()
