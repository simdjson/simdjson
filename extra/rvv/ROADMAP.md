# RVV Backend Roadmap

This document outlines the development stages for the RISC-V Vector (RVV) backend of `simdjson`.

---

## ✅ Phase 1: Foundation (The "Bring-Up")
**Goal:** Establish a stable, correct, and CI-verified backend that passes all unit tests under QEMU.

* **Infrastructure:**
    * [x] Build system integration (CMake toolchains for GCC/Clang).
    * [x] CI pipeline (GitHub Actions) with QEMU emulation.
    * [x] Automated testing scripts (`scripts/rvv/`).
* **Implementation:**
    * [x] `stage1`: Structural indexing (whitespace/operator identification).
    * [x] `stage2`: Tape construction (structural validation).
    * [x] `minify`: Whitespace removal kernel.
    * [x] `validate_utf8`: Vectorized UTF-8 validation.
* **Verification:**
    * [x] Vector Length Agnostic (VLA) compliance (128-1024 bits).
    * [x] Full pass on `ctest` regression suite.

---

## 🚧 Phase 2: Optimization (The "Hot Path")
**Goal:** Optimize critical kernels to approach memory-bandwidth limits on real hardware.

* **Number Parsing:**
    * [ ] Implement vectorized integer parsing (SWAR or vector-mul-add).
    * [ ] Optimize floating-point parsing (reduce scalar fallback overhead).
* **Stage 1 Tuning:**
    * [ ] Experiment with `LMUL=2` or `LMUL=4` for structural indexing to reduce instruction count.
    * [ ] Analyze `vcompress` (permutation) overhead on varying micro-architectures.
* **Base64:**
    * [ ] Implement vectorized Base64 decoding (using table lookups `vrgather`).

---

## 🔮 Phase 3: Advanced Extensions & Hardware
**Goal:** Leverage optional RISC-V extensions and validate on silicon.

* **Zvbb (Vector Bit-manipulation):**
    * Utilize instructions like `vclz` (count leading zeros) or `vwsll` if available for faster parsing logic.
* **On-Demand API:**
    * Deepen integration with the On-Demand front-end to allow lazy partial parsing using vector scanners.
* **Hardware Validation:**
    * Validate performance profiles on:
        * **C908** (T-Head / Allwinner D1-like successors).
        * **SiFive P550/X280**.
        * **Tenstorrent Ascalon**.

---

## 📉 Known Limitations (Current)

1.  **QEMU Timing:** Performance numbers on CI are not representative of real hardware.
2.  **Page Boundaries:** The current `stage1` implementation relies on `vle8ff` (Fault-Only-First) loads. Hardware without robust support for this may see performance penalties if emulated via traps.
3.  **Complex Scalars:** Heavy scalar-to-vector transitions (`vslide1down`) are minimized but still present in the UTF-8 validation slow path.