# RVV Upstreaming Strategy

This document outlines the process for merging the RISC-V Vector (RVV) backend into the upstream `simdjson` repository.

Merging a new architectural backend is a significant event. To ensure a smooth review process and minimize disruption to the existing ecosystem (x86/ARM), we adhere to a **"Phased Integration"** strategy.

---

## 1. The "Do No Harm" Principle

Before opening any Pull Request (PR), the RVV backend must guarantee:

1.  **Zero Regressions:** It must not break builds on x86-64, ARM64, or PPC64.
2.  **Zero Overhead:** It must not add runtime overhead to the detection logic of other architectures.
3.  **Default Off:** Until fully validated, the backend should be behind a CMake flag (`SIMDJSON_ENABLE_RV64`) or disabled by default.

---

## 2. The Phased PR Plan

Do **not** submit a single 10,000-line PR. It will be rejected or languish in review hell. Instead, we split the submission into 4 atomic phases.

### 📦 Phase 1: Infrastructure & Build System
**Scope:** CMake changes, detection logic, and CI scripts.
**Goal:** Teach the build system *how* to build RVV, without actually adding the code yet.

* **Changes:**
    * Modify `CMakeLists.txt` to add `simdjson_rvv` library target (initially empty).
    * Add `cmake/toolchains/rvv-qemu-*.cmake`.
    * Add `scripts/rvv/` helper scripts (optional, maybe keep in a separate dev repo if upstream objects).
    * Update `src/simdjson.cpp` detection logic (e.g., `__riscv_vector` macros).
* **Review Focus:** "Does this break existing builds?"

### 💀 Phase 2: The Skeleton
**Scope:** The empty class structure and header files.
**Goal:** Establish the interface contract.

* **Changes:**
    * Add `src/rvv/implementation.cpp` (Stubbed methods returning `UNIMPLEMENTED`).
    * Add `include/simdjson/rvv/` headers.
    * Register the `rvv` implementation in the runtime registry.
* **Review Focus:** "Does the class hierarchy match the `simdjson` standard?"

### 🧠 Phase 3: The Kernels (The Meat)
**Scope:** The actual vector implementation.
**Goal:** Functionality.

* **Changes:**
    * Implement `stage1` (Structural Indexing).
    * Implement `stage2` (Tape Construction).
    * Implement `minify` and `validate_utf8`.
* **Review Focus:** "Is the code clean, VLA-compliant, and performant?"
* *Note:* This might be split further (e.g., one PR for Stage 1, one for Stage 2) if the diff is too large.

### 🚀 Phase 4: Activation & CI
**Scope:** Turning it on and adding regression tests.
**Goal:** Production readiness.

* **Changes:**
    * Add GitHub Actions workflow (`rvv-qemu-matrix.yml`) to the main repo (or ask maintainers to enable it).
    * Enable the backend by default on RISC-V platforms.
    * Add documentation to `doc/`.
* **Review Focus:** "Is it stable?"

---

## 3. Pre-Submission Checklist

Before submitting **any** PR, verify the following:

- [ ] **Formatting:** Run `clang-format` using the upstream `.clang-format` config.
- [ ] **VLA Compliance:** Verified on QEMU with `VLEN=128`, `256`, and `512`.
- [ ] **Clean Build:** No warnings with `-Wall -Wextra`.
- [ ] **Fallback:** Verified that the code compiles on a RISC-V compiler *without* vector support (should fallback or disable gracefully).

---

## 4. Communication with Maintainers

* **Prefix:** Use `[RVV]` in PR titles (e.g., `[RVV] Add initial CMake toolchain`).
* **Context:** Always explain *why* a change is needed. Most maintainers are experts in x86/ARM but may not know RISC-V specifics (e.g., why `vle8ff` is necessary).
* **Patience:** Emulation-based CI is slow. Be patient with the CI queue.

---

## 5. Maintenance Commitment

By upstreaming this code, we commit to:
1.  **Fixing Bugs:** We are the primary contact for RVV-related issues.
2.  **Monitoring CI:** We watch the QEMU matrix for failures.
3.  **Hardware Validation:** As new RISC-V hardware becomes available, we will validate and tune the backend.