#!/usr/bin/env python3
"""Writes a seed corpus for fuzz/fuzz_sjq.cpp from the golden cases: each seed
is one byte (the filter length), the filter, then the input."""

import os
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_tests  # noqa: E402

out = sys.argv[1]
os.makedirs(out, exist_ok=True)
count = 0
cases = os.path.join(run_tests.HERE, "cases")
for name in sorted(os.listdir(cases)):
    for _, block in run_tests.parse_blocks(os.path.join(cases, name)):
        lines = [l for l in block if not l.startswith("%%")]
        has_file = any(l.startswith("%%FILE") for l in block)
        program = lines[0].encode("utf-8")
        data = b"" if has_file else lines[1].encode("utf-8")
        if len(program) > 255:
            continue
        with open(os.path.join(out, f"seed{count:04d}"), "wb") as f:
            f.write(bytes([len(program)]) + program + data)
        count += 1
print(f"{count} seeds in {out}")
