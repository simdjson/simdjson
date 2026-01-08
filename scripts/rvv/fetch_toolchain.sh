#!/bin/bash
set -euo pipefail

# =============================================================================
# scripts/rvv/fetch_toolchain.sh
# =============================================================================
# RVV Backend: Toolchain Setup (Debian/Ubuntu)
#
# Usage:
#   sudo ./scripts/rvv/fetch_toolchain.sh
#
# Installs:
# - RISC-V cross toolchain + sysroot headers
# - QEMU user-mode (riscv64)
# - Host build tooling (cmake/ninja)
# - Optional host clang/lld (if missing)
#
# Notes:
# - Prefer qemu-user (non-static) when available; qemu-user-static also works.
# - The sysroot is typically /usr/riscv64-linux-gnu (used by qemu -L).
# =============================================================================

# -----------------------------------------------------------------------------
# Common logging helpers (optional)
# -----------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "${SCRIPT_DIR}/common.sh" ]]; then
  # shellcheck disable=SC1091
  source "${SCRIPT_DIR}/common.sh"
else
  log_info() { echo "[INFO] $*"; }
  log_warn() { echo "[WARN] $*" >&2; }
  log_error() { echo "[ERROR] $*" >&2; }
fi

# -----------------------------------------------------------------------------
# Privileges
# -----------------------------------------------------------------------------
if [[ "${EUID}" -ne 0 ]]; then
  log_warn "This script installs packages via apt-get."
  log_warn "Run as root or with sudo."
  exit 1
fi

# -----------------------------------------------------------------------------
# Validate apt-get
# -----------------------------------------------------------------------------
if ! command -v apt-get >/dev/null 2>&1; then
  log_error "This script supports Debian/Ubuntu (apt-get) only."
  log_error "Install equivalents of:"
  log_error "  - riscv64-linux-gnu-gcc / g++"
  log_error "  - libc6-dev-riscv64-cross"
  log_error "  - qemu-user (or qemu-user-static) with riscv64"
  exit 1
fi

# -----------------------------------------------------------------------------
# Install packages
# -----------------------------------------------------------------------------
export DEBIAN_FRONTEND=noninteractive

log_info "Updating package lists..."
apt-get update -q

log_info "Installing RISC-V cross toolchain + sysroot + build tools..."
apt-get install -y -q \
  gcc-riscv64-linux-gnu \
  g++-riscv64-linux-gnu \
  libc6-dev-riscv64-cross \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  git

log_info "Installing QEMU user-mode (riscv64)..."
# Prefer qemu-user (provides qemu-riscv64); also accept qemu-user-static.
apt-get install -y -q qemu-user qemu-user-static

if ! command -v clang >/dev/null 2>&1; then
  log_info "Installing host clang/lld..."
  apt-get install -y -q clang lld
fi

# -----------------------------------------------------------------------------
# Verification
# -----------------------------------------------------------------------------
log_info "Verifying installation..."
ERRORS=0

if command -v riscv64-linux-gnu-g++ >/dev/null 2>&1; then
  VER="$(riscv64-linux-gnu-g++ --version | head -n 1)"
  log_info "GCC cross: FOUND (${VER})"
else
  log_error "GCC cross: MISSING (riscv64-linux-gnu-g++)"
  ERRORS=1
fi

QEMU_BIN=""
if command -v qemu-riscv64 >/dev/null 2>&1; then
  QEMU_BIN="qemu-riscv64"
elif command -v qemu-riscv64-static >/dev/null 2>&1; then
  QEMU_BIN="qemu-riscv64-static"
fi

if [[ -n "${QEMU_BIN}" ]]; then
  VER="$(${QEMU_BIN} --version | head -n 1)"
  log_info "QEMU: FOUND (${QEMU_BIN}) (${VER})"
else
  log_error "QEMU: MISSING (qemu-riscv64 or qemu-riscv64-static)"
  ERRORS=1
fi

# Sysroot hint (used by scripts with qemu -L)
if [[ -d "/usr/riscv64-linux-gnu" ]]; then
  log_info "Sysroot: FOUND (/usr/riscv64-linux-gnu)"
else
  log_warn "Sysroot not found at /usr/riscv64-linux-gnu (this may be OK depending on distro packaging)."
fi

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
echo "--------------------------------------------------"
if [[ "${ERRORS}" -eq 0 ]]; then
  log_info "Toolchain setup complete."
  echo "Next:"
  echo "  - Build:   scripts/rvv/build_rvv_gcc.sh   (or build_rvv_clang.sh)"
  echo "  - Test:    scripts/rvv/run_qemu_ctest.sh  (set QEMU_LD_PREFIX=/usr/riscv64-linux-gnu if needed)"
  echo "  - Bench:   scripts/rvv/run_qemu_benchmark.sh"
  exit 0
else
  log_error "Toolchain setup incomplete (see errors above)."
  exit 1
fi
