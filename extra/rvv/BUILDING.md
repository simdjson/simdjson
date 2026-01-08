
# RVV Backend: Building & Running (Reproducible)

This document is the source of truth for building and running the **simdjson RVV** backend locally in a reproducible way.

Repository root (Windows):
`C:\MyCode\Lemire\simdjson\`

All paths below are relative to repo root.

---

## 0) Requirements

### Toolchains (cross or native)
You need one of:
- **Clang** with RISC-V target support (cross-compiling): `--target=riscv64-linux-gnu`
- **GCC** cross toolchain: `riscv64-linux-gnu-g++`

### QEMU (for local execution on x86 hosts)
- `qemu-riscv64` and/or `qemu-riscv64-static`
- A RISC-V sysroot for dynamic linking (recommended)

### CMake
- CMake >= 3.16 recommended
- Ninja optional but recommended

---

## 1) Fixed variables (do not drift)

These are fixed by the RVV integration spec.

- `IMPLEMENTATION_KEY`: `rvv`
- `MARCH_BASELINE`: `rv64gcv`
- `MARCH_ZVBB`: `rv64gcv_zvbb` (optional)
- `VLEN_LIST`: `128 256 1024`
- QEMU CPU template:
  - `rv64,v=true,vext_spec=v1.0,vlen=${VLEN}`

Build directory template:
- `build/rvv-${COMPILER_ID}-${MARCH}/`
  - Example: `build/rvv-clang-rv64gcv/`

---

## 2) Recommended build modes

We support two primary build modes:

1) **Cross-compile + run under QEMU user-mode** (recommended on x86 hosts)
2) **Native build on a real RISC-V RVV machine** (best performance numbers)

This document focuses on (1) because it is reproducible.

---

## 3) Cross-compile with Clang (recommended)

### 3.1 Configure
From repo root:

```bash
cmake -S . -B build/rvv-clang-rv64gcv -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_FLAGS="--target=riscv64-linux-gnu -march=rv64gcv -mabi=lp64d" \
  -DCMAKE_CXX_FLAGS="--target=riscv64-linux-gnu -march=rv64gcv -mabi=lp64d" \
  -DCMAKE_EXE_LINKER_FLAGS="--target=riscv64-linux-gnu"
````

Notes:

* If you have a sysroot, add:

  * `--sysroot=/path/to/riscv64-sysroot`
* If your sysroot provides dynamic libs, this is sufficient for QEMU user-mode.

### 3.2 Build

```bash
cmake --build build/rvv-clang-rv64gcv -j
```

---

## 4) Cross-compile with GCC

### 4.1 Configure

```bash
cmake -S . -B build/rvv-gcc-rv64gcv -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=riscv64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=riscv64-linux-gnu-g++ \
  -DCMAKE_C_FLAGS="-march=rv64gcv -mabi=lp64d" \
  -DCMAKE_CXX_FLAGS="-march=rv64gcv -mabi=lp64d"
```

### 4.2 Build

```bash
cmake --build build/rvv-gcc-rv64gcv -j
```

---

## 5) Optional: zvbb build

This is an optional build variant. Use only if your toolchain/QEMU supports it.

### Clang

```bash
cmake -S . -B build/rvv-clang-rv64gcv_zvbb -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_FLAGS="--target=riscv64-linux-gnu -march=rv64gcv_zvbb -mabi=lp64d" \
  -DCMAKE_CXX_FLAGS="--target=riscv64-linux-gnu -march=rv64gcv_zvbb -mabi=lp64d"
cmake --build build/rvv-clang-rv64gcv_zvbb -j
```

### GCC

```bash
cmake -S . -B build/rvv-gcc-rv64gcv_zvbb -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=riscv64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=riscv64-linux-gnu-g++ \
  -DCMAKE_C_FLAGS="-march=rv64gcv_zvbb -mabi=lp64d" \
  -DCMAKE_CXX_FLAGS="-march=rv64gcv_zvbb -mabi=lp64d"
cmake --build build/rvv-gcc-rv64gcv_zvbb -j
```

---

## 6) Running tests under QEMU (user-mode)

### 6.1 Environment variables

You may need these depending on your setup:

* `QEMU_CPU` (fixed template):

  * `rv64,v=true,vext_spec=v1.0,vlen=${VLEN}`
* `QEMU_LD_PREFIX` (recommended if using a sysroot):

  * points QEMU to the RISC-V dynamic linker and libraries

Example:

```bash
export QEMU_LD_PREFIX=/path/to/riscv64-sysroot
export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=256"
```

### 6.2 Run ctest

From the build dir:

```bash
ctest --test-dir build/rvv-clang-rv64gcv -j --output-on-failure
```

### 6.3 Run tests with multiple VLEN

Repeat the run with:

* `VLEN=128`
* `VLEN=256`
* `VLEN=1024`

Example loop:

```bash
for VLEN in 128 256 1024; do
  export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=${VLEN}"
  ctest --test-dir build/rvv-clang-rv64gcv -j --output-on-failure
done
```

---

## 7) Running smoketests (RVV bring-up)

After build, run:

* UTF-8 validator smoketest:

  * `tools/rvv/smoke_validate_utf8.cpp` build target name depends on CMake wiring
* Stage1 smoketest:

  * `tools/rvv/smoke_stage1.cpp`

If you configured and built everything with CMake, run the produced executables under QEMU using the same `QEMU_CPU` / `QEMU_LD_PREFIX` environment variables as in section 6.

---

## 8) Benchmarks (reproducible)

Benchmarks output location is fixed:

* `build/rvv-*/bench_results/*.csv`

Use:

* `scripts/rvv/run_qemu_bench.sh`

The script must:

* run the RVV benchmark binaries under QEMU
* print the exact `QEMU_CPU`, `VLEN`, toolchain, and `-march` used
* save CSV outputs under `bench_results/`

---

## 9) Troubleshooting

### 9.1 `qemu-riscv64: Could not open '/lib/ld-linux-riscv64-lp64d.so.1'`

Set a sysroot and export:

```bash
export QEMU_LD_PREFIX=/path/to/riscv64-sysroot
```

### 9.2 Illegal instruction

* Ensure you compiled with `-march=rv64gcv` and that `QEMU_CPU` includes `v=true` and `vext_spec=v1.0`.
* Ensure you are not accidentally running a non-RVV CPU model.

### 9.3 Toolchain does not recognize `rv64gcv`

Update your compiler or install a toolchain that supports RVV 1.0.

---

## 10) One-liner “known good” path

Clang cross-build + QEMU tests at VLEN=256:

```bash
cmake -S . -B build/rvv-clang-rv64gcv -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_FLAGS="--target=riscv64-linux-gnu -march=rv64gcv -mabi=lp64d" \
  -DCMAKE_CXX_FLAGS="--target=riscv64-linux-gnu -march=rv64gcv -mabi=lp64d"
cmake --build build/rvv-clang-rv64gcv -j

export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=256"
# export QEMU_LD_PREFIX=/path/to/riscv64-sysroot
ctest --test-dir build/rvv-clang-rv64gcv -j --output-on-failure
```

