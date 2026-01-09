## RVV Backend Integration Spec (Fixed Names, Paths, Conventions, and Architecture Decisions)

This document is the source of truth for **all new files we will create**, the **fixed identifiers/variables**, and the **locked technical architecture decisions** we will use later when writing code, so we do not drift.

### Repository root

* **REPO_ROOT (Windows):** `C:\MyCode\Lemire\simdjson\`
* **REPO_ROOT (relative paths):** all paths below are relative to repo root.

---

## 1) Fixed identifiers (do not rename)

### Implementation identity

* **Implementation key (string):** `rvv`
* **C++ namespace:** `simdjson::rvv`
* **Implementation class name:** `simdjson::rvv::implementation`
* **Primary compilation unit:** `src/rvv.cpp`
* **Public umbrella header:** `include/simdjson/rvv.h`

### Architecture / feature macros (fixed spellings)

**Existing (already in repo):**

* `SIMDJSON_IS_RVV`
* `SIMDJSON_IS_RISCV64`

**New (we will introduce):**

* `SIMDJSON_IMPLEMENTATION_RVV`
  Compile-time enable for the RVV backend.
* `SIMDJSON_CAN_ALWAYS_RUN_RVV`
  Used by runtime dispatch “supported_by_runtime_system” logic. **Definition:** true when the binary is compiled with RVV enabled (RVV intrinsics available), and false otherwise.
* `SIMDJSON_TARGET_RVV`
  Optional convenience macro used internally in RVV headers only (defined in `include/simdjson/rvv/begin.h`).

### Instruction-set bit

* **New enum value:** `simdjson::internal::instruction_set::RVV`

### Optional RVV extension toggle

* **Existing macro name reserved:** `SIMDJSON_HAS_ZVBB_INTRINSICS`
  We do **not** rely on this being correct for Milestone 2/3. If we use `zvbb`, we will introduce an explicit opt-in macro (name fixed now):
* **New opt-in macro (if/when needed):** `SIMDJSON_RVV_ENABLE_ZVBB` (default: 0)

---

## 2) Fixed directory map (where things live)

### Core implementation directories

* **RVV headers root:** `include/simdjson/rvv/`
* **RVV source:** `src/`

### Bridge/support directories (added for integration convenience)

* **RVV docs:** `extra/rvv/`
* **RVV scripts:** `scripts/rvv/`
* **RVV tools/smoketests:** `tools/rvv/`
* **RVV tests:** `tests/rvv/`

These directories already exist at top-level in the repo scan (e.g., `include`, `src`, `scripts`, `extra`, `tools`, `tests`). This spec only adds **subfolders**.

---

## 3) New files we will create (complete list)

### A) Core RVV backend (required)

**Public umbrella header**

1. `include/simdjson/rvv.h`

**RVV implementation headers** (new directory: `include/simdjson/rvv/`)
2. `include/simdjson/rvv/base.h`
3. `include/simdjson/rvv/begin.h`
4. `include/simdjson/rvv/end.h`
5. `include/simdjson/rvv/intrinsics.h`
6. `include/simdjson/rvv/simd.h`
7. `include/simdjson/rvv/bitmask.h`
8. `include/simdjson/rvv/bitmanipulation.h`
9. `include/simdjson/rvv/stringparsing_defs.h`
10. `include/simdjson/rvv/numberparsing_defs.h`
11. `include/simdjson/rvv/ondemand.h`
12. `include/simdjson/rvv/implementation.h`

**RVV translation unit**
13. `src/rvv.cpp`

**Contract for these files (fixed)**

* `rvv.h` is the only public-facing include users should need to force RVV compilation of that backend.
* `begin.h/end.h` follow the same structure as other backends:

  * define implementation macros (`SIMDJSON_IMPLEMENTATION`, `SIMDJSON_TARGET_RVV`, `SIMDJSON_IMPLEMENTATION_RVV`)
  * include RVV “building blocks” (`base.h`, `intrinsics.h`, `simd.h`, `bitmask.h`, etc.)
  * include the generic kernels (via `rvv.h` including `generic/amalgamated.h`)
  * close namespaces and restore macros in `end.h`.
* `intrinsics.h` is the **only** file that includes `<riscv_vector.h>` directly.
* `simd.h/bitmask.h` provide RVV equivalents of the internal SIMD abstractions used by generic stage1/stage2 code.
* All RVV code uses **modern RVV 1.0 explicit intrinsics** (non-overloaded).

---

### B) Bridge files (strongly recommended for easy evaluation/integration)

**Docs** (new directory: `extra/rvv/`)
14. `extra/rvv/BUILDING.md`
15. `extra/rvv/DESIGN.md`

**Scripts** (new directory: `scripts/rvv/`)
16. `scripts/rvv/build_rvv_clang.sh`
17. `scripts/rvv/build_rvv_gcc.sh`
18. `scripts/rvv/run_qemu_ctest.sh`
19. `scripts/rvv/run_qemu_bench.sh`

**Tools / smoketests** (new directory: `tools/rvv/`)
20. `tools/rvv/smoke_validate_utf8.cpp`
21. `tools/rvv/smoke_stage1.cpp`
22. `tools/rvv/README.md`

---

### C) RVV-focused tests (optional but high-signal)

**Tests** (new directory: `tests/rvv/`)
23. `tests/rvv/rvv_compilation_tests.cpp`
24. `tests/rvv/rvv_utf8_tests.cpp`
25. `tests/rvv/rvv_stage1_tests.cpp`

Minimum viable test footprint is (23) only; (24–25) may be added after initial bring-up.

---

## 4) Existing files we will edit (fixed, minimal “switchboard” set)

This is the **only** set of existing files we touch to connect the backend.

### Build/selection wiring

* `CMakeLists.txt` (or the central CMake file where `SIMDJSON_ALL_IMPLEMENTATIONS` is defined)
* `include/simdjson/builtin/implementation.h` (add RVV selection case)
* `include/simdjson/implementation_detection.h` (define `SIMDJSON_IMPLEMENTATION_RVV`, `SIMDJSON_CAN_ALWAYS_RUN_RVV`, optional `SIMDJSON_RVV_ENABLE_ZVBB` default)

### Runtime ISA detection and instruction set enum

* `include/simdjson/internal/instruction_set.h` (add `RVV`)
* `include/simdjson/internal/isadetection.h` (report RVV support when built with RVV)

### Implementation registry

* `src/implementation.cpp` (register RVV singleton in available implementations)

This set is intentionally small and mechanical.

---

## 5) Fixed build and run matrix (variables locked)

### Compilers (fixed identifiers used in scripts)

* **COMPILER_ID values (fixed):** `clang` or `gcc`
* **Clang target:** `--target=riscv64-linux-gnu`
* **GCC target:** `riscv64-linux-gnu-g++` (or toolchain-provided equivalent)

### ISA flags (baseline + optional)

* **MARCH_BASELINE (fixed):** `rv64gcv`
* **MARCH_ZVBB (fixed):** `rv64gcv_zvbb`

### Intrinsics API (fixed)

* **Intrinsics flavor:** RVV 1.0 **explicit non-overloaded** intrinsics only
  (e.g., `__riscv_vadd_vv_i32m1`, not overloaded `vadd`).

### QEMU runtime variables (scripts will standardize these)

* **VLEN_LIST (fixed):** `128 256 1024`
* **QEMU_CPU template (fixed):** `rv64,v=true,vext_spec=v1.0,vlen=${VLEN}`

### Script outputs (fixed)

* **Build directory template:** `build/rvv-${COMPILER_ID}-${MARCH}/`
  Examples:

  * `build/rvv-clang-rv64gcv/`
  * `build/rvv-gcc-rv64gcv_zvbb/`
* **Benchmark output directory:** `build/rvv-*/bench_results/`
* **Benchmark output format:** `.csv`

---

## 6) Locked RVV architecture decisions (Milestone 2 & 3)

These decisions are fixed now and must not drift.

### 6.1 Block model and LMUL (fixed)

* **RVV_BLOCK_BYTES (fixed):** `64`
* **RVV_LMUL (fixed):** `m1` (LMUL = 1 always)
* **RVV_VL_POLICY (fixed):** For operations on a 64-byte block:

  * Let `HW_VLEN_BYTES` be the hardware vector length in bytes (queried via RVV runtime `vsetvl` behavior).
  * Use `vl = min(HW_VLEN_BYTES, RVV_BLOCK_BYTES)` and iterate as needed to cover 64 bytes when `HW_VLEN_BYTES < 64`.
  * If `HW_VLEN_BYTES >= 64`, run exactly one vector pass per 64-byte block with `vl = 64`.

### 6.2 Mask export strategy (fixed)

* **RVV_MASK_EXPORT (fixed):** `STORE_AND_PACK_U64`

  * Generate predicate mask with RVV compare.
  * Store mask to a small local buffer.
  * Pack into `uint64_t` with stable lane mapping.
* **Not allowed for stage1 baseline:** iterating bits using `vfirst.m` / `vcpop.m` as the primary method.

### 6.3 Tail safety vs padding (fixed)

* **TAIL_SAFETY (fixed):** **Strict / memory-safe**

  * Never issue an RVV load that can read beyond `buf + len + SIMDJSON_PADDING`.
  * The last chunk(s) must use a cleanup strategy (reduce `vl`, masked/partial load, or local copy into an aligned temp buffer).
* **We do not assume** `VLEN <= SIMDJSON_PADDING`.

### 6.4 Shuffle / LUT strategy (fixed baseline)

* **LUT_POLICY (fixed baseline):** **Restrict width for LUT ops**

  * For `lookup_16` / LUT-style classification, operate on fixed 16B/32B slices (microkernel loop) rather than trying to address arbitrary lanes with 8-bit indices across huge vectors.
* A future optimization may introduce replicated-table/broadcast techniques, but baseline code uses restricted-width LUT operations.

### 6.5 ZVBB usage (fixed policy)

* **Baseline:** RVV 1.0 code path does not require `zvbb`.
* **Optional:** If enabled, it must be guarded by `SIMDJSON_RVV_ENABLE_ZVBB == 1`.

---

## 7) Fixed milestone breakdown (what each file set enables)

### Milestone M1 — RVV backend skeleton (compiles + selectable)

* New files: (1–13)
* Minimal glue edits: section 4
* Outcome: RVV implementation exists and can be selected; heavy functions may delegate initially.

### Milestone M2 — RVV UTF-8 validation speedup

* Files primarily affected: `include/simdjson/rvv/simd.h`, `include/simdjson/rvv/bitmask.h`, `src/rvv.cpp`
* Bridge additions: `tools/rvv/smoke_validate_utf8.cpp`, `scripts/rvv/run_qemu_bench.sh`
* Must obey locked architecture decisions in section 6.

### Milestone M3 — RVV Stage 1 structural indexing

* Files primarily affected: `include/simdjson/rvv/simd.h`, `include/simdjson/rvv/bitmask.h`, `include/simdjson/rvv/stringparsing_defs.h`, `src/rvv.cpp`
* Bridge: `tools/rvv/smoke_stage1.cpp`
* Must obey locked architecture decisions in section 6.

---

## 8) Non-negotiable “no drift” rules

1. **No alternative naming:** `rvv` is the only backend key; no `riscv`, no `riscvv`, no `riscv64v`.
2. **Only one public header:** `include/simdjson/rvv.h`.
3. **Only one .cpp TU:** `src/rvv.cpp`.
4. **Only one file includes `<riscv_vector.h>`:** `include/simdjson/rvv/intrinsics.h`.
5. **All bridge scripts live under:** `scripts/rvv/`.
6. **All RVV-specific docs live under:** `extra/rvv/`.
7. **All RVV smoketests live under:** `tools/rvv/`.
8. **LMUL fixed:** always `m1`.
9. **RVV block model fixed:** 64-byte semantic block with `vl = min(hw_vlen_bytes, 64)` and iteration for `hw_vlen_bytes < 64`.
10. **Mask export fixed:** store + pack to `uint64_t`.
11. **Tail fixed:** strict memory-safe tail handling.
12. **LUT fixed baseline:** restricted-width microkernel LUT operations (no “full-width gather” baseline).

---
