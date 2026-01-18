# ==========================================================================
# RVV Backend: GCC Cross-Compilation Toolchain
# ==========================================================================
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/rvv-qemu-gcc.cmake ...
#
# This file configures CMake to use the GCC cross-compiler for RISC-V 64-bit
# (RVV 1.0) on a Linux host (e.g., Ubuntu/Debian).
# ==========================================================================

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# 1. Compilers
# --------------------------------------------------------------------------
# We search for the standard Debian/Ubuntu cross-compiler names.
find_program(CMAKE_C_COMPILER riscv64-linux-gnu-gcc)
find_program(CMAKE_CXX_COMPILER riscv64-linux-gnu-g++)

if(NOT CMAKE_C_COMPILER OR NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "RISC-V GCC cross-compiler not found. Please install 'gcc-riscv64-linux-gnu'.")
endif()

# 2. Target Flags
# --------------------------------------------------------------------------
# -march=rv64gcv: 64-bit, General (IMAFD), Compressed, Vector (V1.0)
# -mabi=lp64d:    Long/Pointers are 64-bit, Doubles passed in FP registers
set(RVV_FLAGS "-march=rv64gcv -mabi=lp64d")

set(CMAKE_C_FLAGS_INIT "${RVV_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${RVV_FLAGS}")

# 3. Search Behavior
# --------------------------------------------------------------------------
# Restrict searches to the target environment (sysroot)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 4. Emulation (Optional)
# --------------------------------------------------------------------------
# Helps try_run() checks succeed during configuration.
find_program(QEMU_EMULATOR qemu-riscv64)
if(QEMU_EMULATOR)
    # Note: We provide a default VLEN of 128 for configuration checks.
    # The actual VLEN used during testing is controlled by the test wrapper script.
    set(CMAKE_CROSSCOMPILING_EMULATOR ${QEMU_EMULATOR} -cpu rv64,v=true,vlen=128 -L /usr/riscv64-linux-gnu)
endif()