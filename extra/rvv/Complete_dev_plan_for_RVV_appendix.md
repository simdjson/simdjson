## Complete development plan (RVV appendix) + locked no-drift variables (consolidated, English)

Goal: finish a “top-expert”, production-ready RVV backend while developing **mostly inside the RVV appendix** (new RVV files: headers/tools/tests/scripts/docs), with **minimal upstream wiring** (“hub” changes) and clean integration (tests, tools, scripts, docs, CI).

---

# 0) Single source of truth (global lock / no drift)

Create one canonical configuration surface and mirror it everywhere:

* **Code truth:** `include/simdjson/rvv/rvv_config.h`
* **Script truth:** `scripts/rvv/common.env` (and `common.sh` / `common.ps1`)
* **Doc truth:** `extra/rvv/CONVENTIONS.md`

These three must contain the **same** values and must be the only place where they are defined.

---

## 0.1 Backend identity (FIXED)

* `IMPL_KEY = "rvv"`
* C++ namespace: `simdjson::rvv`
* Implementation class: `simdjson::rvv::implementation`
* Public header: `include/simdjson/rvv.h`
* Backend headers directory: `include/simdjson/rvv/`
* Primary translation unit: `src/rvv.cpp`

(If you split into multiple `.cpp` files for development, you can squash later for upstream.)

---

## 0.2 Macros (FIXED)

**Existing (repo-level / do not rename):**

* `SIMDJSON_IS_RVV`
* `SIMDJSON_IS_RISCV64`

**RVV backend-local (keep stable):**

* `SIMDJSON_IMPLEMENTATION_RVV`
* `SIMDJSON_CAN_ALWAYS_RUN_RVV`
* `SIMDJSON_TARGET_RVV`

**Optional future fastpaths (default OFF):**

* `SIMDJSON_RVV_ENABLE_ZVBB` (default `0`)
* `SIMDJSON_HAS_ZVBB_INTRINSICS` (if already present, keep name)

---

## 0.3 ISA enum (FIXED)

* `simdjson::internal::instruction_set::RVV`

---

## 0.4 RVV policy (FIXED — architecture decisions)

* `RVV_BLOCK_BYTES = 64`
* `RVV_LMUL = m1` (LMUL=1 only)
* `RVV_VL_POLICY` (for 64B semantic blocks):

  * `vl = vsetvl_e8m1(remaining)` with `remaining <= 64`
  * iterate the 64B block when `hw_vlen_bytes < 64`
  * **never** load more than `remaining` lanes in that micro-iteration
* `RVV_MASK_EXPORT = STORE_AND_PACK_U64` (store mask → pack into `uint64_t`)
* `TAIL_SAFETY = STRICT` (no reads beyond `len + SIMDJSON_PADDING`)
* `LUT_POLICY = RESTRICTED_WIDTH_MICROKERNEL` (16B/32B slices; no full-width gather baseline)
* Intrinsics policy: RVV 1.0 explicit non-overloaded intrinsics only

---

## 0.5 Build & run matrix (FIXED)

* `COMPILER_ID ∈ {clang, gcc}`
* `TARGET_TRIPLE_CLANG = riscv64-linux-gnu`
* `MARCH_BASELINE = rv64gcv`
* `MARCH_ZVBB = rv64gcv_zvbb` (optional)
* `MABI = lp64d`
* `VLEN_LIST_BITS = 128 256 1024`
* `BUILD_DIR = build/rvv-${COMPILER_ID}-${MARCH}/`
* `BENCH_OUT_DIR = build/rvv-*/bench_results/`
* `BENCH_FORMAT = csv` (preferred), otherwise `.txt` in M0

**QEMU_CPU template (must match CI):**

* Canonical template:
  `QEMU_CPU_TEMPLATE = "rv64,v=true,rvv_ta_all_1s=true,rvv_ma_all_1s=true,vlen=${VLEN}"`

If you want `vext_spec=v1.0`, add it **everywhere** (CI + scripts + docs) or nowhere. Do not mix.

---

# 1) File map (role-aligned structure)

## 1.1 Core backend (appendix)

**Public:**

* `include/simdjson/rvv.h`

**Backend headers:**

* `include/simdjson/rvv/base.h`
* `include/simdjson/rvv/begin.h`
* `include/simdjson/rvv/end.h`
* `include/simdjson/rvv/intrinsics.h` (**only** file including `<riscv_vector.h>`)
* `include/simdjson/rvv/bitmask.h`
* `include/simdjson/rvv/bitmanipulation.h`
* `include/simdjson/rvv/simd.h`
* `include/simdjson/rvv/stringparsing_defs.h`
* `include/simdjson/rvv/numberparsing_defs.h`
* `include/simdjson/rvv/ondemand.h`
* `include/simdjson/rvv/implementation.h`
* **ADD (backend parity):** `include/simdjson/rvv/builder.h`

**TU:**

* `src/rvv.cpp`

---

## 1.2 Tools / tests / scripts / docs (appendix)

### Tools

Existing:

* `tools/rvv/README.md`
* `tools/rvv/smoke_validate_utf8.cpp`
* `tools/rvv/smoke_stage1.cpp`

Add for “complete” coverage:

* `tools/rvv/smoke_stage2.cpp`
* `tools/rvv/smoke_minify.cpp`
* `tools/rvv/smoke_ondemand.cpp`
* `tools/rvv/print_rvv_caps.cpp` (dump VLEN/vl/toolchain/QEMU env; anti-drift)

### Tests

Existing:

* `tests/rvv/rvv_compilation_tests.cpp`
* `tests/rvv/rvv_utf8_tests.cpp`
* `tests/rvv/rvv_stage1_tests.cpp`

Add for completeness:

* `tests/rvv/rvv_stage2_tests.cpp`
* `tests/rvv/rvv_minify_tests.cpp`
* `tests/rvv/rvv_ondemand_tests.cpp`
* `tests/rvv/rvv_dom_tests.cpp`
* `tests/rvv/rvv_number_tests.cpp`
* `tests/rvv/rvv_regression_tests.cpp`
* `tests/rvv/rvv_equivalence_tests.cpp` (RVV vs fallback golden equivalence)

### Scripts (Linux)

Existing:

* `scripts/rvv/build_rvv_clang.sh`
* `scripts/rvv/build_rvv_gcc.sh`
* `scripts/rvv/run_qemu_ctest.sh`
* `scripts/rvv/run_qemu_bench.sh`

Add for no-drift + completeness:

* `scripts/rvv/common.sh`
* `scripts/rvv/common.env`
* `scripts/rvv/run_qemu_smoke.sh`
* (optional) `scripts/rvv/fetch_toolchain.sh`
* (optional) `scripts/rvv/verify_toolchain.sh`

### Scripts (Windows / PowerShell) — recommended for production readiness

* `scripts/rvv/build_rvv_clang.ps1`
* `scripts/rvv/build_rvv_gcc.ps1`
* `scripts/rvv/run_qemu_ctest.ps1`
* `scripts/rvv/run_qemu_bench.ps1`
* `scripts/rvv/common.ps1`

### Docs

Existing:

* `extra/rvv/BUILDING.md`
* `extra/rvv/DESIGN.md`

Add for “complete / production-ready”:

* `extra/rvv/CONVENTIONS.md` (no-drift reference values, CPU strings, naming)
* `extra/rvv/CI.md` (exact commands CI runs)
* `extra/rvv/TROUBLESHOOTING.md` (illegal instruction, QEMU, sysroot, vlen)
* `extra/rvv/PERF.md` (methodology, datasets, command lines, reporting)
* `extra/rvv/ROADMAP.md` (phases + exit criteria)
* `extra/rvv/DEBUGGING.md`
* `extra/rvv/UPSTREAMING.md` (how to propose cleanly; PR splits)

### Optional: minimal upstream patch packaging (to avoid “invading” repo code)

* `extra/rvv/patches/0001-wire-rvv.patch`
* `extra/rvv/patches/0002-add-ci-matrix.patch`
* `extra/rvv/patches/0003-singleheader-support.patch`

---

# 2) Full development milestones (“top expert”)

## Milestone M0 — Stabilize structure & eliminate drift (1–2 iterations)

Deliverables:

* Normalize `begin.h`/`end.h`:

  * `SIMDJSON_IMPLEMENTATION` set to `rvv`
  * `SIMDJSON_TARGET_RVV` defined (empty is fine)
  * enforce `<riscv_vector.h>` inclusion only in `intrinsics.h`
* Lock RVV policy in code via constants/wrappers:

  * `static constexpr size_t RVV_BLOCK_BYTES = 64;`
  * `setvl_e8m1(remaining)` wrappers
* Add `include/simdjson/rvv/builder.h` (can initially delegate to generic)
* Add `scripts/rvv/common.sh` (+ `common.env`) and make all RVV scripts consume them
* Add `tools/rvv/print_rvv_caps.cpp`

Files:

* (mod) `include/simdjson/rvv/begin.h`
* (mod) `include/simdjson/rvv/intrinsics.h`
* (new) `include/simdjson/rvv/builder.h`
* (new) `scripts/rvv/common.sh`, `scripts/rvv/common.env`
* (new) `tools/rvv/print_rvv_caps.cpp`
* (mod) `scripts/rvv/build_rvv_*.sh`, `run_qemu_*.sh` to source `common.sh`

---

## Milestone M1 — Solid, fast RVV primitives (SIMD foundations)

Fixed invariants:

* Every 64B operation uses:

  * `for (i=0; i<64; i+=vl) { vl = vsetvl_e8m1(64-i); ... }`
* `to_bitmask(mask, vl)` lane→bit mapping stable
* Any byte read has a strict tail-safe path

Work:

* Expand `simd8x64<uint8_t>` ops as needed: `==`, `<`, `<=`, `&`, `|`, `^`, `~`, reductions, etc.
* Add tail-safe loader:

  * `load_64_or_less(ptr, remaining, out[64])` (local copy + vector loads)
* Harden `bitmask.h` for `vl ∈ [1..64]` (debug asserts)
* Centralize noisy intrinsics in `intrinsics.h` wrappers

Files:

* (mod) `include/simdjson/rvv/simd.h`
* (mod) `include/simdjson/rvv/bitmask.h`
* (mod) `include/simdjson/rvv/intrinsics.h`

---

## Milestone M2 — Real RVV UTF-8 validation (perf + correctness)

Fixed rules:

* `UTF8_BLOCK_BYTES = 64`
* Cross-block state for multi-byte sequences
* Strict tail safety

Work:

* Implement RVV validate_utf8 kernel (classification + range checks + carry state)
* Extend tests and add regression coverage

Files:

* (mod) `src/rvv.cpp`
* (mod) `tools/rvv/smoke_validate_utf8.cpp`
* (mod) `tests/rvv/rvv_utf8_tests.cpp`
* (new) `tests/rvv/rvv_regression_tests.cpp`

---

## Milestone M3 — Real RVV Stage1 structural indexing (core)

Fixed rules:

* mask export = store + pack `uint64_t`
* quote/backslash state carry across blocks
* structurals inside quotes masked out

Work:

* Baseline: comparisons for `{ } [ ] : ,` (acceptable)
* Optimization: 16B LUT microkernel for classification (optional but recommended)
* Keep logic aligned with generic behavior

Files:

* (mod) `include/simdjson/rvv/stringparsing_defs.h`
* (mod) `include/simdjson/rvv/simd.h`
* (mod) `tools/rvv/smoke_stage1.cpp`
* (mod) `tests/rvv/rvv_stage1_tests.cpp`

---

## Milestone M4 — Stage2 (DOM) completion

Work:

* Identify what `generic/stage2` expects in terms of backend hooks
* Optimize:

  * number parsing integration
  * string parsing / escapes
  * whitespace skipping kernels

Files:

* (new) `tests/rvv/rvv_stage2_tests.cpp`
* (new) `tools/rvv/smoke_stage2.cpp`
* (mod) `include/simdjson/rvv/numberparsing_defs.h`
* (mod) `src/rvv.cpp`

---

## Milestone M5 — ondemand RVV (targeted kernels)

Work:

* Implement:

  * `skip_string`
  * `find_next_structural`
  * `skip_value`
* Confirm interfaces with `generic/ondemand`

Files:

* (mod) `include/simdjson/rvv/ondemand.h`
* (new) `tests/rvv/rvv_ondemand_tests.cpp`
* (new) `tools/rvv/smoke_ondemand.cpp`

---

## Milestone M6 — minify RVV (fastpath)

Work:

* whitespace classification (16B microkernel)
* compaction/store strategy (start scalar store, improve later)

Files:

* (new) `tests/rvv/rvv_minify_tests.cpp`
* (new) `tools/rvv/smoke_minify.cpp`
* (mod) `src/rvv.cpp`

---

## Milestone M7 — builder parity (backend completeness)

Work:

* Provide `builder.h` parity (even if delegating early)
* DOM correctness test suite parity

Files:

* (new/mod) `include/simdjson/rvv/builder.h`
* (new) `tests/rvv/rvv_dom_tests.cpp`

---

## Milestone M8 — performance discipline (bench + reports)

Work:

* Make `run_qemu_bench.*` generate stable outputs (csv), include full config metadata (toolchain, commit, march, vlen, qemu)
* Document protocol (datasets, commands, reporting)

Files:

* (mod) `scripts/rvv/run_qemu_bench.sh`
* (new) `extra/rvv/PERF.md`

---

## Milestone M9 — CI matrix + toolchain CMake

Work:

* One workflow: `compiler={clang,gcc}`, `vlen={128,256,1024}`, `march={rv64gcv}`
* Add toolchain files for reproducible builds

Files:

* (new) `.github/workflows/rvv-qemu-matrix.yml`
* (new) `cmake/toolchains/rvv-qemu-clang.cmake`
* (new) `cmake/toolchains/rvv-qemu-gcc.cmake`
* (new) `extra/rvv/CI.md`

---

# 3) Cross-file alignment variables (use everywhere)

## 3.1 CMake target names (FIXED — recommended)

* `TARGET_LIB = simdjson`
* `TARGET_TOOL_VALIDATE_UTF8 = rvv_smoke_validate_utf8`
* `TARGET_TOOL_STAGE1 = rvv_smoke_stage1`
* `TARGET_TOOL_STAGE2 = rvv_smoke_stage2`
* `TARGET_TOOL_MINIFY = rvv_smoke_minify`
* `TARGET_TOOL_ONDEMAND = rvv_smoke_ondemand`
* `TARGET_TEST_COMPILATION = rvv_compilation_tests`
* `TARGET_TEST_UTF8 = rvv_utf8_tests`
* `TARGET_TEST_STAGE1 = rvv_stage1_tests`
* `TARGET_TEST_STAGE2 = rvv_stage2_tests`, etc.

## 3.2 CMake options (FIXED — must not drift)

* Prefer the repo’s existing option if present: `SIMDJSON_ENABLE_RV64=ON`
* If adding an alias `SIMDJSON_ENABLE_RVV`, define it once and map to `SIMDJSON_ENABLE_RV64` to avoid drift.
* Standard toggles:

  * `SIMDJSON_ENABLE_TOOLS`
  * `SIMDJSON_ENABLE_TESTS`
  * `SIMDJSON_ENABLE_BENCHMARKS`

## 3.3 Runtime selection (FIXED)

* Implementation name: `"rvv"`
* Optional env override if supported by simdjson build: `SIMDJSON_FORCE_IMPLEMENTATION=rvv` (otherwise manual selection)

---

# 4) Anti-drift answers (locked decisions)

1. LMUL fixed: **m1**
2. Mask→GPR: **store mask → pack u64** (no vfirst/vcpop baseline)
3. Tail safety: **strict** (no overread, even VLEN=1024)
4. Shuffle/LUT: baseline microkernel **16B/32B** (no full-width gather baseline)
5. Intrinsics: RVV 1.0 **explicit non-overloaded** only
6. `<riscv_vector.h>` included **only** in `include/simdjson/rvv/intrinsics.h`

---

# 5) Optimal coding order (for you + AI)

1. M0 (lock values + common scripts + builder stub + print caps)
2. M1 (primitives)
3. M2 (validate_utf8)
4. M3 (stage1 + optional LUT microkernel)
5. M6 (minify)
6. M4 (stage2)
7. M5 (ondemand)
8. M8 (bench discipline)
9. M9 (CI matrix + toolchains)

---

## Key consolidation notes (what this document fixes vs earlier drafts)

* Adds the missing **backend parity** file: `include/simdjson/rvv/builder.h`.
* Introduces a **single source of truth** (`rvv_config.h` + `common.env` + `CONVENTIONS.md`) to eliminate drift.
* Canonicalizes `QEMU_CPU_TEMPLATE` to match existing CI conventions (or mandates full alignment if you keep `vext_spec=v1.0`).
* Uses the repo-observed CMake option `SIMDJSON_ENABLE_RV64` as the stable knob to avoid introducing new drift.
