#!/bin/bash
set -e

# -------------------------------------------------------------------------
# Configuration: Fixed Run Matrix & Paths
# -------------------------------------------------------------------------
VLEN_LIST="128 256 1024"
BUILD_DIR_PATTERN="build/rvv-*"
BENCH_BINARY="benchmark/bench_simdjson" # Standard simdjson benchmark binary
OUTPUT_DIR="bench_results"

# -------------------------------------------------------------------------
# Validation: Check for QEMU & Build
# -------------------------------------------------------------------------
if ! command -v qemu-riscv64 &> /dev/null; then
    echo "Error: qemu-riscv64 not found."
    echo "Please install 'qemu-user' or 'qemu-user-static'."
    exit 1
fi

# Find build dir (gcc or clang)
BUILD_DIR=$(ls -d ${BUILD_DIR_PATTERN} 2>/dev/null | head -n 1)
if [ -z "$BUILD_DIR" ]; then
    echo "Error: No build directory found. Run build_rvv_clang.sh first."
    exit 1
fi

# Create results directory inside build folder
RESULTS_PATH="${BUILD_DIR}/${OUTPUT_DIR}"
mkdir -p "${RESULTS_PATH}"

echo "==============================================================="
echo "Running RVV Benchmarks via QEMU"
echo "Build Dir: ${BUILD_DIR}"
echo "Results:   ${RESULTS_PATH}"
echo "==============================================================="

# -------------------------------------------------------------------------
# Benchmark Loop
# -------------------------------------------------------------------------

for VLEN in $VLEN_LIST; do
    OUTPUT_FILE="${RESULTS_PATH}/bench_vlen${VLEN}.txt"

    echo "---------------------------------------------------------------"
    echo "Benchmarking VLEN=${VLEN} -> ${OUTPUT_FILE}"
    echo "---------------------------------------------------------------"

    export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=${VLEN}"

    # Check if binary exists
    if [ -f "${BUILD_DIR}/${BENCH_BINARY}" ]; then
        # Run benchmark binary.
        # We assume the default benchmark suite.
        # You can add arguments here like "minify" or "validate_utf8" to filter.
        "${BUILD_DIR}/${BENCH_BINARY}" | tee "${OUTPUT_FILE}"
    else
        echo "Warning: Benchmark binary not found at ${BUILD_DIR}/${BENCH_BINARY}"
        echo "Likely cause: Benchmarks not enabled in CMake (SIMDJSON_ENABLE_BENCHMARKS=ON)."
    fi
done

echo ""
echo "Benchmark suite complete. Results saved in ${RESULTS_PATH}/"
