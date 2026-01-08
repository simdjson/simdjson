#!/bin/bash
set -euo pipefail

# =============================================================================
# scripts/rvv/run_qemu_smoke.sh
# =============================================================================
# Runs RVV smoketest binaries under QEMU user-mode.
#
# Defaults (override via env):
# - VLEN: 128
# - COMPILER: clang
# - QEMU_BIN: qemu-riscv64
# - QEMU_CPU_TEMPLATE (canonical): rv64,v=on,vext_spec=v1.0,vlen=${VLEN},rvv_ta_all_1s=on,rvv_ma_all_1s=on
# - QEMU_LD_PREFIX: optional (sysroot)
#
# Build dir selection:
# - If BUILD_DIR is set, uses it
# - Else uses common.sh get_build_dir(compiler,vlen) if available
# - Else falls back to build/rvv-* (first match, sorted)
#
# Implementation forcing:
# - Exports both SIMDJSON_FORCE_IMPLEMENTATION and SIMDJSON_FORCE_IMPL to "rvv"
# =============================================================================

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------
die() { echo "Error: $*" >&2; exit 1; }
need_cmd() { command -v "$1" >/dev/null 2>&1 || die "Missing required command: $1"; }

render_qemu_cpu() {
  local vlen="$1"
  local tmpl="${QEMU_CPU_TEMPLATE:-rv64,v=on,vext_spec=v1.0,vlen=\${VLEN},rvv_ta_all_1s=on,rvv_ma_all_1s=on}"
  # shellcheck disable=SC2016
  VLEN="${vlen}" eval "echo \"${tmpl}\""
}

pick_build_dir_fallback() {
  local pattern="${BUILD_DIR_PATTERN:-build/rvv-*}"
  local first
  first="$(ls -1d ${pattern} 2>/dev/null | sort | head -n 1 || true)"
  [[ -n "${first}" ]] || die "No build directory found matching '${pattern}'. Build first (scripts/rvv/build_rvv_clang.sh or gcc)."
  echo "${first}"
}

# -----------------------------------------------------------------------------
# Load optional common.sh (if present)
# -----------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_SH="${SCRIPT_DIR}/common.sh"
if [[ -f "${COMMON_SH}" ]]; then
  # shellcheck disable=SC1090
  source "${COMMON_SH}"
fi

# -----------------------------------------------------------------------------
# Args / defaults
# -----------------------------------------------------------------------------
COMPILER="${1:-${COMPILER:-clang}}"
VLEN="${2:-${VLEN:-128}}"

QEMU_BIN="${QEMU_BIN:-qemu-riscv64}"
need_cmd "${QEMU_BIN}"

# Build dir resolution order:
# 1) BUILD_DIR env
# 2) common.sh get_build_dir if it exists
# 3) fallback glob
if [[ -n "${BUILD_DIR:-}" ]]; then
  [[ -d "${BUILD_DIR}" ]] || die "BUILD_DIR is set but not a directory: ${BUILD_DIR}"
elif declare -F get_build_dir >/dev/null 2>&1; then
  BUILD_DIR="$(get_build_dir "${COMPILER}" "${VLEN}")"
else
  BUILD_DIR="$(pick_build_dir_fallback)"
fi
[[ -d "${BUILD_DIR}" ]] || die "Build directory not found: ${BUILD_DIR}"

# QEMU CPU string:
# Prefer common.sh get_qemu_cpu if defined, else render from template.
if declare -F get_qemu_cpu >/dev/null 2>&1; then
  CPU_STR="$(get_qemu_cpu "${VLEN}")"
else
  CPU_STR="$(render_qemu_cpu "${VLEN}")"
fi

# Sysroot / loader:
# If QEMU_LD_PREFIX is set, use it. Otherwise, do not assume distro paths.
QEMU_ARGS=(-cpu "${CPU_STR}")
if [[ -n "${QEMU_LD_PREFIX:-}" ]]; then
  QEMU_ARGS+=(-L "${QEMU_LD_PREFIX}")
fi

# Force RVV implementation (avoid silent fallback)
export SIMDJSON_FORCE_IMPLEMENTATION=rvv
export SIMDJSON_FORCE_IMPL=rvv

# -----------------------------------------------------------------------------
# Targets (override by setting TARGETS env: space-separated list)
# -----------------------------------------------------------------------------
if [[ -n "${TARGETS:-}" ]]; then
  # shellcheck disable=SC2206
  TARGET_LIST=(${TARGETS})
else
  TARGET_LIST=(
    "print_rvv_caps"
    "rvv_smoke_minify"
    "rvv_smoke_stage2"
    "rvv_smoke_ondemand"
    "rvv_smoke_numberparse"
  )
fi

echo "--------------------------------------------------"
echo "RVV Smoketests (QEMU user-mode)"
echo "Compiler: ${COMPILER}"
echo "VLEN:     ${VLEN}"
echo "Build:    ${BUILD_DIR}"
echo "QEMU:     ${QEMU_BIN}"
echo "QEMU_CPU: ${CPU_STR}"
if [[ -n "${QEMU_LD_PREFIX:-}" ]]; then
  echo "Sysroot:  ${QEMU_LD_PREFIX}"
fi
echo "--------------------------------------------------"

# -----------------------------------------------------------------------------
# Execution loop
# -----------------------------------------------------------------------------
FAIL_COUNT=0
PASS_COUNT=0
SKIP_COUNT=0

for TARGET in "${TARGET_LIST[@]}"; do
  BINARY=""

  POSSIBLE_PATHS=(
    "${BUILD_DIR}/${TARGET}"
    "${BUILD_DIR}/bin/${TARGET}"
    "${BUILD_DIR}/tools/${TARGET}"
    "${BUILD_DIR}/tools/rvv/${TARGET}"
  )

  for CANDIDATE in "${POSSIBLE_PATHS[@]}"; do
    if [[ -f "${CANDIDATE}" ]]; then
      BINARY="${CANDIDATE}"
      break
    fi
  done

  if [[ -z "${BINARY}" ]]; then
    echo "[SKIP] ${TARGET}: binary not found"
    ((SKIP_COUNT++))
    continue
  fi

  if [[ ! -x "${BINARY}" ]]; then
    echo "[SKIP] ${TARGET}: not executable (${BINARY})"
    ((SKIP_COUNT++))
    continue
  fi

  echo ""
  echo ">>> Running ${TARGET}"
  echo "    ${QEMU_BIN} ${QEMU_ARGS[*]} ${BINARY}"
  if "${QEMU_BIN}" "${QEMU_ARGS[@]}" "${BINARY}"; then
    echo "[PASS] ${TARGET}"
    ((PASS_COUNT++))
  else
    echo "[FAIL] ${TARGET}"
    ((FAIL_COUNT++))
  fi
  echo "--------------------------------------------------"
done

echo ""
echo "Summary: ${PASS_COUNT} passed, ${FAIL_COUNT} failed, ${SKIP_COUNT} skipped"
if [[ "${FAIL_COUNT}" -eq 0 ]]; then
  exit 0
else
  exit 1
fi
