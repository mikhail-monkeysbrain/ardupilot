import unittest
from vtx_table_normalize import normalize_cli

BASE = "vtxtable bands 1\nvtxtable channels 1\nvtxtable band 1 TEST T CUSTOM 5800\n"

class PowerCLITests(unittest.TestCase):
    def test_codes_and_labels(self):
        t = normalize_cli(BASE + "vtxtable powerlevels 4\nvtxtable powervalues 0 1 2 3\nvtxtable powerlabels 25 500 1000 2500\n")
        self.assertEqual(t["powerlevels"], [{"value": 0, "label": "25"}, {"value": 1, "label": "500"}, {"value": 2, "label": "1000"}, {"value": 3, "label": "2500"}])

    def test_text_labels(self):
        t = normalize_cli(BASE + "vtxtable powerlevels 2\nvtxtable powervalues 7 16\nvtxtable powerlabels LOW MAX\n")
        self.assertEqual(t["powerlevels"][1], {"value": 16, "label": "MAX"})

    def test_missing_values(self):
        with self.assertRaises(ValueError):
            normalize_cli(BASE + "vtxtable powerlevels 2\nvtxtable powerlabels LOW MAX\n")

    def test_wrong_count(self):
        with self.assertRaises(ValueError):
            normalize_cli(BASE + "vtxtable powerlevels 2\nvtxtable powervalues 1\nvtxtable powerlabels LOW MAX\n")

    def test_invalid_code(self):
        with self.assertRaises(ValueError):
            normalize_cli(BASE + "vtxtable powerlevels 1\nvtxtable powervalues 65536\nvtxtable powerlabels MAX\n")

    def test_empty_table(self):
        self.assertEqual(normalize_cli(BASE + "vtxtable powerlevels 0\n")["powerlevels"], [])

if __name__ == "__main__":
    unittest.main()
