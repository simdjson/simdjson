# RVV Backend Conventions

This document defines the architectural baseline, coding standards, and design philosophy for the `simdjson` RISC-V Vector (RVV) backend.

All contributions to the `src/rvv/` directory must adhere to these conventions to ensure portability across different RISC-V hardware implementations (e.g., QEMU, C908, X280, Ara).

---

## 1. Architectural Baseline

We target the **RISC-V Vector Extension v1.0** (Ratified).

* **Architecture String:** `rv64gcv`
    * `rv64`: 64-bit address space.
    * `g`: General-purpose scalar instructions (IMAFD).
    * `c`: Compressed instructions (optional but standard).
    * `v`: Vector Extension v1.0.
* **Minimum VLEN:** 128 bits.
    * Code **must** work correctly on `VLEN=128`.
    * Code **should** scale automatically to larger VLENs (256, 512, 1024, etc.).
    * Code is **not** required to support `VLEN=64` (Embedded profile) at this time.

### Compiler Flags
The build system standardizes on:
```bash
-march=rv64gcv -mabi=lp64d