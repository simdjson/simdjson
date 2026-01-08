# RVV Debugging Guide

This document provides tactical instructions for debugging the RISC-V Vector backend using QEMU and GDB.

---

## 1. The Debugging Loop

The most effective workflow for fixing RVV crashes or logic errors is:

1.  **Isolate:** Create a minimal reproduction (repro) using the smoke tools.
2.  **Disassemble:** Verify the compiler is generating the vector instructions you expect.
3.  **Trace:** Use QEMU logging to see the execution flow.
4.  **Debug:** Attach GDB to inspect vector registers state.

---

## 2. Step 1: Isolation (The Smoke Tools)

Don't debug the full `ctest` suite. It takes too long to load in QEMU. Instead, use the standalone tools in `tools/rvv/`.

**Example: Debugging a segmentation fault in Stage 1.**
Compile with debug symbols (`-g`):
```bash
# Force a debug build for better GDB experience
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DSIMDJSON_ENABLE_RV64=ON
cmake --build build/debug --target rvv_smoke_minify