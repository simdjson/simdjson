#!/bin/bash
set -e

# -------------------------------------------------------------------------
# Configuration: Fixed Paths and Flags (per Spec)
# -------------------------------------------------------------------------
# We use the fixed spec variables for compiler and target architecture.
# This ensures consistency with the QEMU runtime scripts.

COMPILER_ID="clang"
TARGET_TRIPLE="riscv64-linux-gnu"
MARCH_BASELINE="rv64gcv"
BUILD_DIR="build/rvv-${COMPILER_ID}-${MARCH_BASELINE}"

# -------------------------------------------------------------------------
# Validation: Check for Clang
# -------------------------------------------------------------------------
if ! command -v clang++ &> /dev/null; then
    echo "Error: clang++ not found in PATH."
    exit 1
fi

echo "==============================================================="
echo "Building simdjson for RVV (${MARCH_BASELINE})"
echo "Compiler: clang++ (Target: ${TARGET_TRIPLE})"
echo "Output:   ${BUILD_DIR}"
echo "==============================================================="

# -------------------------------------------------------------------------
# CMake Configuration
# -------------------------------------------------------------------------
# -DSIMDJSON_ENABLE_RVV=ON : Forces our backend checks (if applicable in CMake)
# -DCMAKE_CROSSCOMPILING=ON : Often helps with test exclusions
# -DBUILD_SHARED_LIBS=OFF   : Static linkage is easier for QEMU user-mode emulation

cmake -S . -B "${BUILD_DIR}" \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_FLAGS="--target=${TARGET_TRIPLE} -march=${MARCH_BASELINE} -static" \
    -DCMAKE_C_FLAGS="--target=${TARGET_TRIPLE} -march=${MARCH_BASELINE} -static" \
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