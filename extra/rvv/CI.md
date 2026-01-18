# RVV Continuous Integration (CI)

This document details the automated testing infrastructure for the RISC-V Vector (RVV) backend.

The CI pipeline is designed to enforce **Vector Length Agnosticism (VLA)** by validating the code across multiple synthetic Vector Lengths (VLEN) using QEMU User Mode Emulation.

---

## 1. The Strategy: "Trust but Verify"

Writing VLA code is difficult. A kernel that works on your local machine (likely VLEN=128) might crash on a machine with VLEN=512 or VLEN=1024 due to incorrect strip-mining or buffer overruns.

To solve this, our CI matrix explodes every commit into multiple virtual environments:



| Variable | Values | Purpose |
| :--- | :--- | :--- |
| **Compiler** | `clang`, `gcc` | Catch compiler-specific bugs or strictness differences. |
| **VLEN** | `128`, `256`, `512`, `1024` | Verify VLA compliance. |
| **Emulator** | `qemu-riscv64` | User-mode emulation for `rv64gcv`. |

---

## 2. Workflows

### A. Smoke Test (`rvv-smoke.yml`)
* **Trigger:** Push/PR to `src/rvv/`, `scripts/rvv/`, etc.
* **Goal:** Fast feedback (< 2 mins).
* **Config:** `Clang` + `VLEN=128` (Standard Profile).
* **What runs:** `scripts/rvv/run_qemu_smoke.sh` (Tools only).

### B. The Matrix (`rvv-qemu-matrix.yml`)
* **Trigger:** Push/PR to `main`.
* **Goal:** Correctness verification.
* **Config:** Matrix of `[GCC, Clang]` × `[128, 256, 512]`.
* **What runs:** Full CTest suite (`scripts/rvv/run_qemu_ctest.sh`).

### C. Nightly Deep Scan (`rvv-nightly-matrix.yml`)
* **Trigger:** Schedule (02:00 UTC).
* **Goal:** Stress testing & Future proofing.
* **Config:** Adds **VLEN=1024** and runs full benchmarks.
* **Why separate?** VLEN=1024 emulation is extremely slow; running it on every PR would clog the queue.

---

## 3. Local Reproduction

You can reproduce **any** CI failure on your local Linux machine (Ubuntu/Debian recommended) using the `scripts/rvv/` folder.

### Step 1: Setup Environment
Install the cross-compiler and QEMU:
```bash
sudo ./scripts/rvv/fetch_toolchain.sh
./scripts/rvv/verify_toolchain.sh