#!/usr/bin/env python3
"""
Amalgamates simdjson into temporary files, then builds and runs 7 feature-flag
test programs.  Reports compile time and binary size for each configuration.
"""

import os
import sys
import subprocess
import tempfile
import time
import shutil

SCRIPTPATH = os.path.dirname(os.path.abspath(__file__))
PROJECTPATH = os.path.dirname(SCRIPTPATH)
TESTSRC = os.path.join(PROJECTPATH, "tests", "feature_flag_tests")

# ── feature-flag configurations ──────────────────────────────────────────────
CONFIGS = [
    {
        "name": "dom_only",
        "flags": {"SIMDJSON_FEATURE_DOM_API": 1,
                  "SIMDJSON_FEATURE_ONDEMAND_API": 0,
                  "SIMDJSON_FEATURE_BUILDER_API": 0},
    },
    {
        "name": "ondemand_only",
        "flags": {"SIMDJSON_FEATURE_DOM_API": 1,
                  "SIMDJSON_FEATURE_ONDEMAND_API": 1,
                  "SIMDJSON_FEATURE_BUILDER_API": 0},
    },
    {
        "name": "dom_and_ondemand",
        "flags": {"SIMDJSON_FEATURE_DOM_API": 1,
                  "SIMDJSON_FEATURE_ONDEMAND_API": 1,
                  "SIMDJSON_FEATURE_BUILDER_API": 0},
    },
    {
        "name": "builder_and_dom",
        "flags": {"SIMDJSON_FEATURE_DOM_API": 1,
                  "SIMDJSON_FEATURE_ONDEMAND_API": 0,
                  "SIMDJSON_FEATURE_BUILDER_API": 1},
    },
    {
        "name": "builder_and_ondemand",
        "flags": {"SIMDJSON_FEATURE_DOM_API": 1,
                  "SIMDJSON_FEATURE_ONDEMAND_API": 1,
                  "SIMDJSON_FEATURE_BUILDER_API": 1},
    },
    {
        "name": "builder_dom_and_ondemand",
        "flags": {"SIMDJSON_FEATURE_DOM_API": 1,
                  "SIMDJSON_FEATURE_ONDEMAND_API": 1,
                  "SIMDJSON_FEATURE_BUILDER_API": 1},
    },
    {
        "name": "builder_only",
        "flags": {"SIMDJSON_FEATURE_DOM_API": 1,
                  "SIMDJSON_FEATURE_ONDEMAND_API": 0,
                  "SIMDJSON_FEATURE_BUILDER_API": 1},
    },
]

CXX = os.environ.get("CXX", "c++")
CXXSTD = os.environ.get("CXXSTD", "-std=c++17")
OPT = os.environ.get("OPT", "-O2")


def amalgamate(output_dir):
    """Run amalgamate.py, writing simdjson.h / simdjson.cpp into *output_dir*."""
    env = os.environ.copy()
    env["AMALGAMATE_OUTPUT_PATH"] = output_dir
    cmd = [sys.executable, os.path.join(SCRIPTPATH, "amalgamate.py")]
    result = subprocess.run(cmd, env=env, capture_output=True, text=True)
    if result.returncode != 0:
        print("amalgamate.py failed:")
        print(result.stdout)
        print(result.stderr)
        sys.exit(1)
    header = os.path.join(output_dir, "simdjson.h")
    source = os.path.join(output_dir, "simdjson.cpp")
    assert os.path.isfile(header), f"missing {header}"
    assert os.path.isfile(source), f"missing {source}"
    return header, source


def compile_and_run(name, flags, amal_dir, build_dir):
    """
    1. Compile simdjson.cpp -> simdjson.o  (timed, size reported)
    2. Compile test .cpp -> test.o         (timed, size reported)
    3. Link and run the test program

    Returns (lib_time, lib_size, test_time, test_size) or Nones on failure.
    """
    src = os.path.join(TESTSRC, f"{name}.cpp")
    simdjson_cpp = os.path.join(amal_dir, "simdjson.cpp")
    simdjson_o = os.path.join(build_dir, f"{name}_simdjson.o")
    test_o = os.path.join(build_dir, f"{name}.o")
    binary = os.path.join(build_dir, name)

    defs = [f"-D{k}={v}" for k, v in flags.items()]

    # ── compile simdjson.o (timed) ────────────────────────────────────────
    cmd_lib = [CXX, CXXSTD, OPT, "-c"] + defs + [
        f"-I{amal_dir}", "-o", simdjson_o, simdjson_cpp,
    ]
    t0 = time.monotonic()
    result = subprocess.run(cmd_lib, capture_output=True, text=True)
    lib_time = time.monotonic() - t0

    if result.returncode != 0:
        print(f"  COMPILE FAILED (simdjson.o): {name}")
        print(f"  command: {' '.join(cmd_lib)}")
        print(result.stderr)
        return None, None, None, None

    lib_size = os.path.getsize(simdjson_o)

    # ── compile test source (timed) ───────────────────────────────────────
    cmd_test = [CXX, CXXSTD, OPT, "-c"] + defs + [
        f"-I{amal_dir}", "-o", test_o, src,
    ]
    t0 = time.monotonic()
    result = subprocess.run(cmd_test, capture_output=True, text=True)
    test_time = time.monotonic() - t0

    if result.returncode != 0:
        print(f"  COMPILE FAILED (test): {name}")
        print(f"  command: {' '.join(cmd_test)}")
        print(result.stderr)
        return None, None, None, None

    test_size = os.path.getsize(test_o)

    # ── link ──────────────────────────────────────────────────────────────
    cmd_link = [CXX, "-o", binary, test_o, simdjson_o]
    result = subprocess.run(cmd_link, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  LINK FAILED: {name}")
        print(f"  command: {' '.join(cmd_link)}")
        print(result.stderr)
        return None, None, None, None

    # ── run ───────────────────────────────────────────────────────────────
    result = subprocess.run([binary], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  RUN FAILED: {name}")
        print(result.stdout)
        print(result.stderr)
        return lib_time, lib_size, test_time, test_size

    print(f"  {result.stdout.strip()}")
    return lib_time, lib_size, test_time, test_size


def human_size(n):
    """Return e.g. '1.23 MB' or '456 KB'."""
    if n is None:
        return "N/A"
    if n >= 1_000_000:
        return f"{n / 1_000_000:.2f} MB"
    if n >= 1_000:
        return f"{n / 1_000:.1f} KB"
    return f"{n} B"


def main():
    tmpdir = tempfile.mkdtemp(prefix="simdjson_features_")
    amal_dir = os.path.join(tmpdir, "amalgamated")
    build_dir = os.path.join(tmpdir, "build")
    os.makedirs(amal_dir, exist_ok=True)
    os.makedirs(build_dir, exist_ok=True)

    print(f"Working directory: {tmpdir}")
    print(f"Compiler: {CXX}  flags: {CXXSTD} {OPT}")
    print()

    # ── amalgamate ────────────────────────────────────────────────────────
    print("Amalgamating...")
    amalgamate(amal_dir)
    print("Amalgamation done.\n")

    # ── build & run each configuration ────────────────────────────────────
    results = []
    for cfg in CONFIGS:
        name = cfg["name"]
        flags = cfg["flags"]
        enabled = [k.replace("SIMDJSON_FEATURE_", "").replace("_API", "")
                   for k, v in flags.items() if v]
        label = " + ".join(enabled) if enabled else "none"
        print(f"[{name}] ({label})")
        lib_time, lib_size, test_time, test_size = \
            compile_and_run(name, flags, amal_dir, build_dir)
        results.append((name, label, lib_time, lib_size, test_time, test_size))

    print()

    # ── summary table ─────────────────────────────────────────────────────
    name_w = max(len(r[0]) for r in results)
    label_w = max(len(r[1]) for r in results)
    hdr = (f"{'Test':<{name_w}}  {'APIs':<{label_w}}"
           f"  {'lib time':>9}  {'simdjson.o':>11}"
           f"  {'test time':>10}  {'test .o':>11}")
    print(hdr)
    print("-" * len(hdr))
    for name, label, lt, ls, tt, ts in results:
        lt_s = f"{lt:.2f} s" if lt is not None else "FAIL"
        ls_s = human_size(ls)
        tt_s = f"{tt:.2f} s" if tt is not None else "FAIL"
        ts_s = human_size(ts)
        print(f"{name:<{name_w}}  {label:<{label_w}}"
              f"  {lt_s:>9}  {ls_s:>11}"
              f"  {tt_s:>10}  {ts_s:>11}")

    # ── cleanup ───────────────────────────────────────────────────────────
    shutil.rmtree(tmpdir, ignore_errors=True)

    # exit non-zero if anything failed
    if any(lt is None for _, _, lt, _, _, _ in results):
        print("\nSome tests FAILED.")
        sys.exit(1)
    print("\nAll tests passed.")


if __name__ == "__main__":
    main()
