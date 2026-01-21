
# tools/rvv — RVV Bring-up Tools (Smoketests)

This directory contains **small, fast, self-contained executables** used to validate the RVV backend during bring-up.

They are intentionally:
- quick to build
- quick to run
- easy to execute under QEMU user-mode
- useful for bisecting regressions

Repo root (Windows): `C:\MyCode\Lemire\simdjson\`

All paths below are relative to repo root.

---

## 0) Fixed variables (do not drift)

From the RVV integration spec:

- `IMPLEMENTATION_KEY`: `rvv`
- `MARCH_BASELINE`: `rv64gcv`
- `MARCH_ZVBB`: `rv64gcv_zvbb` (optional)
- `VLEN_LIST`: `128 256 1024`
- `QEMU_CPU` template:
  - `rv64,v=true,vext_spec=v1.0,vlen=${VLEN}`

---

## 1) Tools in this folder

### 1.1 `smoke_validate_utf8`
Source: `tools/rvv/smoke_validate_utf8.cpp`

Purpose:
- Sanity-check that the RVV build is runnable.
- Validate correctness of the UTF-8 validator.
- Provide a fast micro performance signal (not a full benchmark).

Expected behavior:
- Returns exit code `0` on success.
- Prints a short summary:
  - implementation name selected
  - file size processed
  - validation result
  - throughput estimate (optional)

### 1.2 `smoke_stage1`
Source: `tools/rvv/smoke_stage1.cpp`

Purpose:
- Sanity-check stage1 structural indexing behavior on a small set of representative JSON inputs.
- Verify the RVV mask export strategy (store+pack) matches fallback.

Expected behavior:
- Returns exit code `0` on success.
- Prints:
  - selected implementation
  - which test cases ran
  - pass/fail for each case

---

## 2) Building

These tools should be built by the top-level CMake build (recommended).

Example (clang cross build):
```bash
cmake -S . -B build/rvv-clang-rv64gcv -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_FLAGS="--target=riscv64-linux-gnu -march=rv64gcv -mabi=lp64d" \
  -DCMAKE_CXX_FLAGS="--target=riscv64-linux-gnu -march=rv64gcv -mabi=lp64d"
cmake --build build/rvv-clang-rv64gcv -j
````

If you use a different build directory, keep the directory naming convention:

* `build/rvv-${COMPILER_ID}-${MARCH}/`

---

## 3) Running under QEMU (user-mode)

### 3.1 Required environment variables

At minimum set `QEMU_CPU`:

```bash
export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=256"
```

If you are dynamically linked and using a sysroot, set `QEMU_LD_PREFIX`:

```bash
export QEMU_LD_PREFIX=/path/to/riscv64-sysroot
```

### 3.2 Running executables

The actual executable names/locations depend on CMake wiring. Two common patterns:

* `build/.../tools/rvv/<exe>`
* `build/.../<exe>`

Run them with the same QEMU env:

```bash
export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=256"

# Example paths (adjust to where CMake places binaries)
qemu-riscv64 ./build/rvv-clang-rv64gcv/tools/rvv/smoke_validate_utf8
qemu-riscv64 ./build/rvv-clang-rv64gcv/tools/rvv/smoke_stage1
```

### 3.3 Run across all VLENs

```bash
for VLEN in 128 256 1024; do
  export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=${VLEN}"
  qemu-riscv64 ./build/rvv-clang-rv64gcv/tools/rvv/smoke_validate_utf8
  qemu-riscv64 ./build/rvv-clang-rv64gcv/tools/rvv/smoke_stage1
done
```

---

## 4) Expected output (high-level)

The tools should print:

* selected simdjson implementation (must include `rvv` when built/selected)
* a clear PASS/FAIL summary
* a short diagnostic message on failure

Failures should:

* print the exact test input or filename (when applicable)
* return non-zero exit code
* avoid large dumps unless `--verbose` is provided

---

## 5) Design notes

* These tools are not intended to be “benchmarks”.
* They should not require network access.
* They should run in < 1 second for small embedded test inputs.
* They are designed to catch:

  * illegal instruction issues
  * incorrect QEMU CPU configuration
  * obvious RVV mask export mapping bugs
  * stage1 quote/backslash parity mismatches

---

## 6) Quick checklist

Before reporting a bug:

1. Confirm `-march=rv64gcv` (or `_zvbb` if applicable).
2. Confirm `QEMU_CPU` includes `v=true` and `vext_spec=v1.0`.
3. Run both smoketests at `VLEN=128` and `VLEN=1024`.
4. If dynamic linking: confirm `QEMU_LD_PREFIX` points to a valid RISC-V sysroot.
