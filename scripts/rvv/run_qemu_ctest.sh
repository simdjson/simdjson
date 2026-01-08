#!/usr/bin/env bash
set -euo pipefail

# -----------------------------------------------------------------------------
# scripts/rvv/run_qemu_ctest.sh
# -----------------------------------------------------------------------------
# Runs ctest under QEMU user-mode across a fixed VLEN matrix.
#
# Fixed by spec:
# - VLEN_LIST: 128 256 1024
# - QEMU_CPU template: rv64,v=true,vext_spec=v1.0,vlen=${VLEN}
# - Build dir template: build/rvv-${COMPILER_ID}-${MARCH}/
#
# Behavior:
# - Picks a build dir deterministically:
#   1) If BUILD_DIR is set, uses it.
#   2) Else uses first match of build/rvv-* (sorted).
# - Uses CTEST_PARALLEL_LEVEL when set, else nproc.
# - Preserves caller's working directory.
# - Optionally supports dynamic sysroot via QEMU_LD_PREFIX (caller sets it).
# -----------------------------------------------------------------------------

VLEN_LIST=("128" "256" "1024")
BUILD_DIR_PATTERN="build/rvv-*"

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------
die() { echo "Error: $*" >&2; exit 1; }

need_cmd() { command -v "$1" >/dev/null 2>&1 || die "Missing required command: $1"; }

detect_jobs() {
  if [[ -n "${CTEST_PARALLEL_LEVEL:-}" ]]; then
    echo "${CTEST_PARALLEL_LEVEL}"
    return
  fi
  if command -v nproc >/dev/null 2>&1; then
    nproc
    return
  fi
  echo 1
}

pick_build_dir() {
  if [[ -n "${BUILD_DIR:-}" ]]; then
    [[ -d "${BUILD_DIR}" ]] || die "BUILD_DIR is set but not a directory: ${BUILD_DIR}"
    echo "${BUILD_DIR}"
    return
  fi

  # Deterministic choice: lexicographically first directory.
  local first
  first="$(ls -1d ${BUILD_DIR_PATTERN} 2>/dev/null | sort | head -n 1 || true)"
  [[ -n "${first}" ]] || die "No build directory found matching '${BUILD_DIR_PATTERN}'. Run scripts/rvv/build_rvv_clang.sh (or gcc) first."
  echo "${first}"
}

# -----------------------------------------------------------------------------
# Validation
# -----------------------------------------------------------------------------
need_cmd qemu-riscv64
need_cmd ctest

BUILD_DIR="$(pick_build_dir)"
JOBS="$(detect_jobs)"

echo "==============================================================="
echo "Running RVV Tests via QEMU (user-mode)"
echo "Build Dir: ${BUILD_DIR}"
echo "VLEN List: ${VLEN_LIST[*]}"
echo "Jobs:      ${JOBS}"
if [[ -n "${QEMU_LD_PREFIX:-}" ]]; then
  echo "QEMU_LD_PREFIX: ${QEMU_LD_PREFIX}"
fi
echo "==============================================================="

# -----------------------------------------------------------------------------
# Test Loop
# -----------------------------------------------------------------------------
FAILED=0
ROOT_DIR="$(pwd)"

for VLEN in "${VLEN_LIST[@]}"; do
  echo ""
  echo "---------------------------------------------------------------"
  echo "Testing with VLEN=${VLEN} bits"
  echo "QEMU_CPU=rv64,v=true,vext_spec=v1.0,vlen=${VLEN}"
  echo "---------------------------------------------------------------"

  export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=${VLEN}"

  # Run in subshell to avoid cwd leakage.
  if ! (cd "${BUILD_DIR}" && ctest --output-on-failure -j "${JOBS}"); then
    echo "FAILURE: Tests failed with VLEN=${VLEN}" >&2
    FAILED=1
  fi
done

cd "${ROOT_DIR}" >/dev/null 2>&1 || true

# -----------------------------------------------------------------------------
# Final Summary
# -----------------------------------------------------------------------------
echo ""
if [[ "${FAILED}" -eq 0 ]]; then
  echo "SUCCESS: All tests passed across all VLEN configurations."
  exit 0
else
  echo "FAILURE: Some tests failed."
  exit 1
fi
