# RVV Backend: Troubleshooting Guide

This guide addresses common issues encountered when developing, building, or running the RISC-V Vector (RVV) backend for `simdjson`.

---

## 1. Compilation Errors

### `fatal error: riscv_vector.h: No such file or directory`
**Cause:** Your compiler does not support RVV 1.0 intrinsics or the target architecture flags are missing.
**Fix:**
1.  Ensure you are using **GCC >= 10** or **Clang >= 12** (ideally Clang 14+ or GCC 11+).
2.  Verify the compiler flags include `-march=rv64gcv`.
3.  On Ubuntu/Debian, install `gcc-riscv64-linux-gnu`.

### `error: unrecognized command line option '-march=rv64gcv'`
**Cause:** The compiler is too old to recognize the `v` (Vector) extension in the architecture string.
**Fix:** Upgrade your toolchain. If using a system package manager is not an option, consider building a distinct toolchain or using a Docker container.

### Linker Errors: `undefined reference to '__riscv_vsetvl_e8m1'`
**Cause:** You are linking objects compiled with different architecture flags, or the linker is not using the correct libgcc/compiler-rt.
**Fix:** Ensure `CMAKE_CXX_FLAGS` includes `-march=rv64gcv` during both compilation and linking. Use the provided toolchain files (`cmake/toolchains/rvv-qemu-*.cmake`).

---

## 2. Runtime Crashes (QEMU)

### `SIGILL` (Illegal Instruction)
**Cause:** The executable contains instructions the emulator does not understand.
1.  **Old QEMU:** You need QEMU >= 6.0 (ideally 7.0+) for RVV 1.0 support.
2.  **Wrong CPU:** You ran `qemu-riscv64` without enabling vectors.
**Fix:**
Always use the wrapper scripts (`scripts/rvv/run_qemu_*.sh`) which inject:
```bash
-cpu rv64,v=true,vlen=128