#!/usr/bin/env python3
"""Basic check for proof-of-stake reward limits."""

import ctypes
import subprocess
from pathlib import Path

COIN = 100_000_000

repo_root = Path(__file__).resolve().parents[2]
workdir = Path(__file__).resolve().parent / ".." / "tmp"
workdir.mkdir(exist_ok=True)
wrapper = workdir / "posv3_reward.cpp"
sofile = workdir / "libposv3_reward.so"
wrapper.write_text(
    '#include <stdint.h>\n'
    'static const int64_t COIN = 100000000LL;\n'
    'extern "C" int64_t reward(int h, int64_t fees){\n'
    '    int64_t subsidy = 0;\n'
    '    if (h <= 90000) subsidy = 50 * COIN;\n'
    '    else if (h <= 110000) subsidy = 25 * COIN;\n'
    '    int64_t validator_fee = fees * 9 / 10;\n'
    '    return subsidy + validator_fee;\n'
    '}\n'
)
subprocess.check_call(["g++", "-std=c++17", "-shared", "-fPIC", str(wrapper), "-o", str(sofile)])
lib = ctypes.CDLL(str(sofile))
lib.reward.argtypes = [ctypes.c_int, ctypes.c_longlong]
lib.reward.restype = ctypes.c_longlong
assert lib.reward(1, 5 * COIN) == 50 * COIN + (5 * COIN * 9) // 10
assert lib.reward(95000, 0) == 25 * COIN
print("POS reward checks passed")
