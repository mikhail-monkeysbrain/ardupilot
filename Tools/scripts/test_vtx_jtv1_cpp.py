#!/usr/bin/env python3
"""Host-only cross-language JTV1 parser test. Requires a C++11 compiler."""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))
from vtx_table_normalize import normalize_json
from vtx_table_pack import pack_table

CPP = r"""
#include <AP_VideoTX/AP_VTXTableJTV1.h>
#include <AP_VideoTX/AP_VTXTableReceiver.h>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
int main(int argc, char** argv) {
    if (argc != 3) return 3;
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) return 4;
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    JTV1::Table table{};
    table.bands = 99;
    JTV1::Receiver receiver;
    if (receiver.complete() || receiver.validate(table)) return 5;
    // Reassemble out of order; include duplicate retransmission.
    for (size_t end = bytes.size(); end != 0;) {
        const size_t start = end > 31 ? end - 31 : 0;
        if (!receiver.write(start, bytes.data() + start, end - start)) return 6;
        if (!receiver.write(start, bytes.data() + start, end - start)) return 7;
        end = start;
    }
    if (!receiver.complete()) return 8;
    bool ok = receiver.validate(table);
    if (argv[2][0] == '1') {
        if (!ok || table.bands != 3 || table.channels != 8 || table.powers != 2 ||
            table.frequency[2][0] != 4020 || table.frequency[2][4] != 0 ||
            table.power[1].protocol_code != 4 || table.power[1].mw != 0) return 1;
    } else {
        if (ok || table.bands != 99) return 2;
    }
    return 0;
}
"""


class CrossLanguageTests(unittest.TestCase):
    def test_python_pack_cpp_decode(self):
        table = normalize_json({"vtx_table": {
            "bands_list": [
                {"name": "A", "letter": "A", "is_factory_band": False, "frequencies": [3700]},
                {"name": "B", "letter": "B", "is_factory_band": False, "frequencies": [3860]},
                {"name": "C", "letter": "C", "is_factory_band": False, "frequencies": [4020, 4040, 4060, 4080, 0]}],
            "powerlevels_list": [{"value": 1, "label": "MIN"}, {"value": 4, "label": "MAX"}]}})
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            source = directory / "decode.cpp"
            binary = directory / "decode"
            payload = directory / "vtx.bin"
            source.write_text(CPP)
            subprocess.run(["g++", "-std=c++11", "-Wall", "-Wextra", "-Werror",
                            "-I", str(ROOT / "libraries"), str(source), "-o", str(binary)],
                           check=True)
            raw = pack_table(table)
            payload.write_bytes(raw)
            subprocess.run([str(binary), str(payload), "1"], check=True)
            broken = bytearray(raw)
            broken[20] ^= 0x01
            payload.write_bytes(broken)
            subprocess.run([str(binary), str(payload), "0"], check=True)


if __name__ == "__main__":
    unittest.main()
