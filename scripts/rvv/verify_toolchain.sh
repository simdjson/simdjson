#!/bin/bash
# ==========================================================================
# RVV Backend: Toolchain Verification
# ==========================================================================
# Usage: ./verify_toolchain.sh [compiler_bin]
#   compiler_bin: Optional path to C++ compiler (default: riscv64-linux-gnu-g++)
#
# This script compiles a minimal RVV 1.0 program and executes it under QEMU
# to verify the environment is ready for development.
# ==========================================================================

# 1. Source Common Config
# --------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

# 2. Settings
# --------------------------------------------------------------------------
CXX=${1:-"riscv64-linux-gnu-g++"}
TEMP_DIR=$(mktemp -d)
TEST_SRC="${TEMP_DIR}/rvv_check.cpp"
TEST_BIN="${TEMP_DIR}/rvv_check.exe"

log_info "Verifying toolchain..."
log_info "  Compiler: $CXX"

# 3. Create Minimal RVV 1.0 Test Case
# --------------------------------------------------------------------------
# This code checks for:
# - <riscv_vector.h> availability
# - __riscv_vsetvl_e8m1 intrinsic (RVV 1.0 specific)
cat > "$TEST_SRC" <<EOF
#include <iostream>
#if defined(__riscv_vector)
  #include <riscv_vector.h>
#endif

int main() {
#if defined(__riscv_vector)
    // Request VL for 10 elements, 8-bit, LMUL=1
    size_t vl = __riscv_vsetvl_e8m1(10);
    std::cout << "RVV OK. VL=" << vl << std::endl;
    return 0;
#else
    std::cout << "RVV MACRO MISSING" << std::endl;
    return 1;
#endif
}
EOF

# 4. Step 1: Compilation
# --------------------------------------------------------------------------
# We use the baseline architecture defined in common.env
# -march=rv64gcv -mabi=lp64d
echo -n "  [1/2] Compiling minimal RVV test case ... "

if $CXX -march=${MARCH_BASELINE} -mabi=${MABI} -O2 -o "$TEST_BIN" "$TEST_SRC" > /dev/null 2>&1; then
    echo "PASS"
else
    echo "FAIL"
    log_error "Compilation failed. Your compiler might not support -march=${MARCH_BASELINE} or <riscv_vector.h>."
    rm -rf "$TEMP_DIR"
    exit 1
fi

# 5. Step 2: Emulation (Runtime)
# --------------------------------------------------------------------------
echo -n "  [2/2] Running under QEMU ... "

# Resolve QEMU command
if ! command -v qemu-riscv64 &> /dev/null; then
    echo "FAIL"
    log_error "qemu-riscv64 not found in PATH."
    rm -rf "$TEMP_DIR"
    exit 1
fi

# We use VLEN=128 for verification as it's the standard minimum
CPU_STR=$(get_qemu_cpu 128)
QEMU_CMD="qemu-riscv64 -cpu ${CPU_STR} -L /usr/riscv64-linux-gnu"

# Capture output
OUTPUT=$($QEMU_CMD "$TEST_BIN")
RET=$?

if [ $RET -eq 0 ] && [[ "$OUTPUT" == *"RVV OK"* ]]; then
    echo "PASS"
    log_info "Verification Successful: Toolchain and Emulator are ready."
    log_info "  Detected Output: $OUTPUT"
else
    echo "FAIL"
    log_error "Runtime check failed."
    log_error "  Exit Code: $RET"
    log_error "  Output:    $OUTPUT"
    log_error "  Command:   $QEMU_CMD $TEST_BIN"
    rm -rf "$TEMP_DIR"
    exit 1
fi

# 6. Cleanup
# --------------------------------------------------------------------------
rm -rf "$TEMP_DIR"
exit 0