#!/usr/bin/env python3
"""Offline tests for Betaflight CLI/JSON and canonical VTX table conversion."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from vtx_table_normalize import normalize_cli, normalize_json

CLI = """vtxtable bands 3
vtxtable channels 8
vtxtable band 1 BAND_A A FACTORY 4500 4520 4540 4560 4580 4600 4620 4640
vtxtable band 2 BAND_B B FACTORY 4660 4680 4700 4720 4740 4760 4780 4800
vtxtable band 3 BAND_E E FACTORY 4820 4840 4860 4880 0 0 0 0
save
"""
JSON = {
    "description": "Betaflight VTX Config file",
    "version": "1.0",
    "vtx_table": {
        "bands_list": [
            {"name": "BAND_A", "letter": "A", "is_factory_band": False,
             "frequencies": [3700, 3720, 3740, 3760, 3780, 3800, 3820, 3840]},
            {"name": "BAND_B", "letter": "B", "is_factory_band": False,
             "frequencies": [3860, 3880, 3900, 3920, 3940, 3960, 3980, 4000]},
            {"name": "BAND_C", "letter": "C", "is_factory_band": False,
             "frequencies": [4020, 4040, 4060, 4080]},
        ],
        "powerlevels_list": [
            {"value": 1, "label": "AUTO"},
            {"value": 2, "label": "MIN"},
            {"value": 3, "label": "MID"},
            {"value": 4, "label": "MAX"},
        ],
    },
}


class NormalizerTests(unittest.TestCase):
    def test_cli_example_preserves_disabled_channels(self):
        table = normalize_cli(CLI)
        self.assertEqual(len(table["bands"]), 3)
        self.assertEqual(table["bands"][2]["frequencies"], [4820, 4840, 4860, 4880, 0, 0, 0, 0])
        self.assertTrue(table["bands"][0]["is_factory_band"])

    def test_json_example_preserves_power_codes(self):
        table = normalize_json(JSON)
        self.assertEqual(table["bands"][2]["frequencies"], [4020, 4040, 4060, 4080])
        self.assertEqual([p["value"] for p in table["powerlevels"]], [1, 2, 3, 4])
        self.assertEqual(table["powerlevels"][3]["label"], "MAX")

    def test_canonical_round_trip(self):
        table = normalize_json(JSON)
        self.assertEqual(normalize_json(table), table)

    def test_reject_out_of_range_frequency(self):
        bad = {"vtx_table": {"bands_list": [
            {"name": "A", "letter": "A", "is_factory_band": False, "frequencies": [9000]}]}}
        with self.assertRaises(ValueError):
            normalize_json(bad)

    def test_reject_wrong_channel_count(self):
        with self.assertRaisesRegex(ValueError, "frequency count"):
            normalize_cli(CLI.replace("4820 4840 4860 4880 0 0 0 0", "4820 4840"))


if __name__ == "__main__":
    unittest.main()
