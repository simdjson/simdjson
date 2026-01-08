# ==========================================================================
# RVV Backend: Clang Cross-Compilation Toolchain
# ==========================================================================
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/rvv-qemu-clang.cmake ...
#
# This file configures CMake to use Clang for cross-compiling to RISC-V 64-bit
# (RVV 1.0) on a Linux host (e.g., Ubuntu/Debian).
# ==========================================================================

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# 1. Target Triple
# --------------------------------------------------------------------------
# This tells Clang which target architecture and ABI to generate code for.
# "riscv64-unknown-linux-gnu" is the standard tuple for Linux RISC-V.
set(TARGET_TRIPLE "riscv64-unknown-linux-gnu")

# 2. Compilers
# --------------------------------------------------------------------------
# We assume 'clang' and 'clang++' are in the PATH.
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# 3. Target Flags
# --------------------------------------------------------------------------
# We enforce the target triple and the specific RISC-V architecture flags here.
# -march=rv64gcv: 64-bit, General (IMAFD), Compressed, Vector (V1.0)
# -mabi=lp64d:    Long/Pointers are 64-bit, Doubles passed in FP registers
set(RVV_FLAGS "--target=${TARGET_TRIPLE} -march=rv64gcv -mabi=lp64d")

# Note: We append to INIT flags to ensure they are present for all configurations
set(CMAKE_C_FLAGS_INIT "${RVV_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${RVV_FLAGS}")

# 4. Search Behavior
# --------------------------------------------------------------------------
# When finding libraries/headers, look in the target environment (sysroot)
# rather than the host system.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 5. Sysroot / GCC Toolchain Detection (Optional Helper)
# --------------------------------------------------------------------------
# Clang usually finds the installed GCC cross-toolchain automatically if
# the 'gcc-riscv64-linux-gnu' package is installed.
# If you face linking errors (missing crt*.o), you may need to explicitly
# point to the GCC toolchain here:
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --gcc-toolchain=/usr/lib/gcc-cross/riscv64-linux-gnu/12")

# 6. Emulation (Optional)
# --------------------------------------------------------------------------
# Helps try_run() checks succeed during configuration by running them via QEMU.
# Note: We default to basic qemu-riscv64. The specific CPU model (vlen)
# is usually handled by the runtime wrapper script, but this helps basic checks.
find_program(QEMU_EMULATOR qemu-riscv64)
if(QEMU_EMULATOR)
    set(CMAKE_CROSSCOMPILING_EMULATOR ${QEMU_EMULATOR} -cpu rv64,v=true,vlen=128 -L /usr/riscv64-linux-gnu)
endif()