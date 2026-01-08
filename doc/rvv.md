
# RVV backend (RISC-V Vector) — bring-up, build, test, and CI

This document describes the **simdjson RVV backend** (RISC-V Vector Extension, RVV v1.0) as integrated in this tree: build wiring, QEMU user-mode execution, tests, tools, benchmarks, fuzzing, and the “fixed variables” that must not drift.

> Scope: this is **engineering documentation** for the RVV port (bring-up + maintenance).  
> It is not a performance paper; use `benchmark/` for performance results.

---

## Fixed variables (do not drift)

These are “contract” values used across scripts/tools/CI.

- **Implementation key:** `rvv`
- **Baseline march:** `rv64gcv`
- **Optional march (feature-gated):** `rv64gcv_zvbb` (only when explicitly enabled)
- **Semantic block size:** `64` bytes (stage1 logical chunk)
- **VLEN matrix (QEMU validation):** `128 256 1024`
- **QEMU CPU template:**
  - `rv64,v=true,vext_spec=v1.0,vlen=${VLEN}`
- **Force implementation (runtime):**
  - `SIMDJSON_FORCE_IMPLEMENTATION=rvv`

---

## What “RVV backend” means in simdjson

In simdjson terms, a backend implementation:

1. Registers itself in the runtime registry (name/description/instruction set).
2. Provides a `dom_parser_implementation` specialization (stage1 + stage2 glue).
3. Exposes standalone kernels used by public APIs:
   - `minify(...)`
   - `validate_utf8(...)`
4. Ensures runtime selection can pick it (and that it can be forced for CI).

The RVV backend is **VLEN-agnostic**: it must work for multiple hardware vector lengths, while preserving simdjson’s fixed **64-byte logical block** processing model.

---

## File map (RVV-related)

### Public include entry points

- `include/simdjson/rvv.h`  
  Public include that wires RVV generic instantiations into `simdjson::rvv` and provides method definitions when needed.

- `include/simdjson/rvv/implementation.h`  
  Declares `simdjson::rvv::implementation` (registered backend object).

### RVV backend internals (include/)

- `include/simdjson/rvv/intrinsics.h`  
  RVV intrinsics wrapper. Must be safe to include in non-RVV builds (guarded by `SIMDJSON_IS_RVV`) and must not shadow standard RVV typedefs.

- `include/simdjson/rvv/simd.h`  
  SIMD abstractions used by RVV stage1 kernels (e.g., simd8/simd8x64 scaffolding).

- `include/simdjson/rvv/rvv_config.h`  
  Single source of truth for RVV constants (block bytes, LMUL lock, etc.).

- `include/simdjson/rvv/base.h`, `include/simdjson/rvv/begin.h`, `include/simdjson/rvv/end.h`, `include/simdjson/rvv/builder.h`  
  Standard simdjson backend structure and glue.

### Library implementation (src/)

- `src/rvv.cpp` (or `src/rvv/implementation.cpp` depending on your layout)  
  Provides non-header-only definitions for the RVV `implementation` virtuals (factory + minify + utf8).

### Tests

- `tests/rvv/*.cpp`  
  Backend-focused test suite (registry checks, equivalence vs fallback, stage1 stress, minify tests, UTF-8 tests, corpus tests, etc.).

### Tools (smoketests)

- `tools/rvv/smoke_stage1.cpp`
- `tools/rvv/smoke_validate_utf8.cpp`
- (optional bring-up tools) `tools/rvv/print_rvv_caps.cpp`, etc.

### Scripts (QEMU + cross builds)

- `scripts/rvv/build_rvv_clang.sh`
- `scripts/rvv/build_rvv_gcc.sh`
- `scripts/rvv/run_qemu_smoke.sh`
- `scripts/rvv/run_qemu_ctest.sh`
- `scripts/rvv/run_qemu_benchmark.sh`
- `scripts/rvv/fetch_toolchain.sh`
- `scripts/rvv/common.sh` (helpers)

### CI

- `.github/workflows/rvv.yml`

### Benchmarks (optional but recommended)

- `benchmarks/rvv_stage1_bench.cpp`
- `benchmarks/rvv_minify_bench.cpp`
- `benchmarks/rvv_utf8_bench.cpp`

### Fuzzing (optional but recommended)

- `fuzz/rvv_fuzz_parse.cpp`
- `fuzz/rvv_fuzz_minify.cpp`
- `fuzz/rvv_fuzz_validate_utf8.cpp`

---

## Build

### Recommended (cross, static, script)

These scripts are designed so QEMU user-mode execution is easy (static linking).

```bash
# Clang cross build
./scripts/rvv/build_rvv_clang.sh

# GCC cross build
./scripts/rvv/build_rvv_gcc.sh
````

Expected build directory naming convention:

* `build/rvv-${COMPILER_ID}-${MARCH}/`

  * Example: `build/rvv-clang-rv64gcv/`
  * Example: `build/rvv-gcc-rv64gcv/`

### Manual CMake (reference)

Key points:

* Enable the backend: `-DSIMDJSON_ENABLE_RV64=ON`
* Ensure RVV code compiles with `-march=rv64gcv` (and appropriate ABI)
* Prefer static (`-static`) for QEMU user-mode simplicity

---

## Running under QEMU (user-mode)

### Minimum environment

```bash
export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=128"
export SIMDJSON_FORCE_IMPLEMENTATION=rvv
```

If you built dynamically, you’ll also need a RISC-V sysroot:

```bash
export QEMU_LD_PREFIX=/usr/riscv64-linux-gnu
```

### Smoketests

Run the RVV bring-up tools:

```bash
qemu-riscv64 ./build/rvv-clang-rv64gcv/tools/rvv/smoke_validate_utf8
qemu-riscv64 ./build/rvv-clang-rv64gcv/tools/rvv/smoke_stage1
```

### Full test matrix (VLEN)

The RVV backend must work across the fixed VLEN list:

```bash
for VLEN in 128 256 1024; do
  export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=${VLEN}"
  qemu-riscv64 ./build/rvv-clang-rv64gcv/tools/rvv/smoke_validate_utf8
  qemu-riscv64 ./build/rvv-clang-rv64gcv/tools/rvv/smoke_stage1
done
```

Or use the helper:

```bash
./scripts/rvv/run_qemu_ctest.sh
```

---

## Tests philosophy (what each layer catches)

### `tools/rvv/*` (smoketests)

Fast, bring-up oriented, meant for rapid iteration and QEMU debugging.

* **Stage1 smoketest:** quote/escape correctness, “structurals in strings”, boundary padding.
* **UTF-8 smoketest:** truncation/overlong/surrogates, tail handling.

### `tests/rvv/*` (regression + deeper coverage)

More exhaustive, still backend-focused:

* Registry / instantiation / macro validation.
* Stage1 boundaries and escape parity.
* Minify correctness + boundary sweep.
* On-demand integration sanity.
* Equivalence vs fallback on representative cases.
* Optional corpus parsing when datasets are present.

---

## Benchmarks (recommended additions)

If you add RVV-specific microbenchmarks:

* Make them opt-in (or guarded behind benchmark enable flag).
* Ensure they print:

  * active backend
  * vlen (when running under QEMU)
  * input sizes / iterations

Suggested benchmarks:

1. **Stage1 structural scanning**: measure stage1 throughput on fixed inputs.
2. **Minify**: whitespace removal kernel only (not full parse).
3. **UTF-8**: validation kernel only (valid/invalid mixes).

These should not replace `benchmark/bench_simdjson`—they complement it.

---

## Fuzzing (recommended additions)

Fuzzing is a great way to catch:

* tail handling bugs
* boundary-condition bugs
* mask export bugs
* undefined behavior issues under unusual inputs

Recommended fuzz targets:

1. Parse fuzz: feeds random JSON-ish payloads to DOM and/or on-demand entry points.
2. Minify fuzz: random bytes as input (should never crash; correctness depends on contract).
3. UTF-8 fuzz: random bytes + validity oracle (where available) or at least “no crash”.

Run fuzzers under QEMU only if practical; often it’s better to fuzz on native hardware. For RVV-specific behavior, you can still run shorter fuzz campaigns in QEMU as a sanity check.

---

## CI (`.github/workflows/rvv.yml`) expectations

A good RVV CI workflow typically:

* installs cross toolchain + qemu user-mode
* configures a static cross build with `SIMDJSON_ENABLE_RV64=ON`
* runs:

  * smoketests (fast signal)
  * a small subset of `ctest` (or full `ctest` when feasible)
  * VLEN matrix sweep (128/256/1024)
* forces backend:

  * `SIMDJSON_FORCE_IMPLEMENTATION=rvv`

CI should clearly fail if RVV isn’t selected—silent fallback defeats the purpose.

---

## Troubleshooting guide

### “rvv backend not found”

* Build did not compile RVV sources:

  * check `-DSIMDJSON_ENABLE_RV64=ON`
  * check that RVV sources are added to the target
  * check the compile definitions used to register RVV (e.g., `SIMDJSON_IMPLEMENTATION_RVV=1`)

### SIGILL under QEMU

* `QEMU_CPU` missing vector enablement or v1.0 spec:

  * must include `v=true,vext_spec=v1.0`
* binary built with instructions not supported by QEMU version:

  * try upgrading QEMU (newer is generally better for RVV)

### Tests pass but you suspect fallback ran

* set:

  * `export SIMDJSON_FORCE_IMPLEMENTATION=rvv`
* ensure tests print active backend name early.

### Dynamic linker / missing libraries under QEMU

* prefer `-static`
* or set `QEMU_LD_PREFIX` to a valid riscv64 sysroot

---

## Maintenance checklist

Before landing RVV changes:

1. Build with both clang and gcc scripts (when available).
2. Run smoketests under QEMU for **VLEN=128 and VLEN=1024** (and ideally full `128/256/1024`).
3. Run `ctest` under QEMU (or at least the `tests/rvv/*` subset).
4. Confirm the backend is forced during tests (no silent fallback).
5. Keep “fixed variables” stable unless you intentionally rev the spec.

---

## Future work (common “next” items)

* Expand simd8/simd8x64 operations beyond scaffolding as RVV stage1 matures.
* Add optional `zvbb` fast paths behind a feature macro and build option.
* Improve benchmark coverage and add regression datasets for stage1/utf8 corner cases.
* Add sanitizers on native RVV hardware (when available) to catch UB.

---


