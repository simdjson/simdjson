#!/bin/bash
set -e

# -------------------------------------------------------------------------
# Configuration: Fixed Paths and Flags (per Spec)
# -------------------------------------------------------------------------
# We use the fixed spec variables for compiler and target architecture.
COMPILER_BIN="riscv64-linux-gnu-g++"
C_COMPILER_BIN="riscv64-linux-gnu-gcc"
MARCH_BASELINE="rv64gcv"
BUILD_DIR="build/rvv-gcc-${MARCH_BASELINE}"

# -------------------------------------------------------------------------
# Validation: Check for GCC Toolchain
# -------------------------------------------------------------------------
if ! command -v "${COMPILER_BIN}" &> /dev/null; then
    echo "Error: ${COMPILER_BIN} not found in PATH."
    echo "Please install the cross-compiler (e.g., sudo apt install g++-riscv64-linux-gnu)."
    exit 1
fi

echo "==============================================================="
echo "Building simdjson for RVV (${MARCH_BASELINE})"
echo "Compiler: ${COMPILER_BIN}"
echo "Output:   ${BUILD_DIR}"
echo "==============================================================="

# -------------------------------------------------------------------------
# CMake Configuration
# -------------------------------------------------------------------------
# -static : Critical for QEMU user-mode emulation without library path headaches.

cmake -S . -B "${BUILD_DIR}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_BIN}" \
    -DCMAKE_C_COMPILER="${C_COMPILER_BIN}" \
    -DCMAKE_CXX_FLAGS="-march=${MARCH_BASELINE} -static" \
    -DCMAKE_C_FLAGS="-march=${MARCH_BASELINE} -static" \
    -DSIMDJSON_BUILD_STATIC=ON \
    -DSIMDJSON_ENABLE_TESTS=ON \
    -DSIMDJSON_ENABLE_RVV=ON \
    -DCMAKE_BUILD_TYPE=Release

# -------------------------------------------------------------------------
# Build
# -------------------------------------------------------------------------
echo "Compiling..."
cmake --build "${BUILD_DIR}" -j$(nproc)

echo "==============================================================="
echo "Build Complete."
echo "To run tests via QEMU, use: ./scripts/rvv/run_qemu_ctest.sh"
echo "==============================================================="