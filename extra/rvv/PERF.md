# RVV Performance Guide

This document outlines the methodology for benchmarking and tuning the `simdjson` RVV backend.

---

## 1. The Golden Rule: Emulation vs. Silicon

**⚠️ CRITICAL WARNING ⚠️**

**Do NOT trust timing results from QEMU.**

* **QEMU (User Mode):** Is a functional emulator. It translates RISC-V instructions to host (x86/ARM) instructions via TCG (Tiny Code Generator).
    * *Good for:* Verifying that optimized kernels do not crash.
    * *Good for:* Counting exact instruction retirements (via plugins).
    * *Useless for:* Measuring GB/s throughput. A "fast" vector instruction in QEMU might just be a slow helper function loop in C.

* **Real Silicon:** (e.g., Kendryte K230, Lichee Pi 4A, SiFive HiFive Pro P550).
    * This is where truth lives. Optimization decisions must ultimately be validated here.

---

## 2. Running Benchmarks

### A. On QEMU (Sanity Check)
Use the provided script to ensure the benchmark harness runs without crashing. This validates that your "high-performance" loop doesn't segfault after 10,000 iterations.

```bash
# Runs benchmarks with a very short timeout
./scripts/rvv/run_qemu_bench.sh clang 128