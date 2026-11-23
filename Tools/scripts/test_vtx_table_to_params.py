#!/usr/bin/env python3
"""Offline tests for full 88-channel AP_Param persistence export."""
import unittest
from vtx_table_normalize import normalize_json
from vtx_table_to_params import checksum, parameters


class ParameterTableTests(unittest.TestCase):
    def test_88_channels_and_commit_last(self):
        bands = [{"name": "X", "letter": "X", "is_factory_band": False,
                  "frequencies": [1200 + b * 100 + c for c in range(8)]}
                 for b in range(11)]
        table = normalize_json({"bands": bands})
        rows = parameters(table)
        self.assertEqual(len(rows), 89)
        self.assertEqual(rows[-1], ("VTX_TCNT", 11))
        self.assertFalse(any(name == "VTX_TCRC" for name, _ in rows))
        self.assertEqual(dict(rows)["VTX_TB10_C8"], 2207)
        self.assertTrue(0 < checksum(table) <= 0xffffff)

    def test_missing_channels_are_zero(self):
        table = normalize_json({"bands": [{"name": "A", "letter": "A",
                               "is_factory_band": False, "frequencies": [6200]}]})
        params = dict(parameters(table))
        self.assertEqual(params["VTX_TB0_C1"], 6200)
        self.assertEqual(params["VTX_TB0_C2"], 0)
        self.assertEqual(params["VTX_TB10_C8"], 0)

    def test_power_codes_not_silently_dropped(self):
        table = normalize_json({"bands": [{"name": "A", "letter": "A",
                               "is_factory_band": False, "frequencies": [5800]}],
                               "powerlevels": [{"value": 1, "label": "MAX"}]})
        with self.assertRaisesRegex(ValueError, "power"):
            parameters(table)


if __name__ == "__main__":
    unittest.main()
