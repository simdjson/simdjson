#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

// 1. Include Simdjson Configuration
// We include the public header to check high-level feature macros
#include "simdjson.h"

// We include the RVV specific config to check backend policy locks
// (This path assumes the tool is built within the repo structure)
#include "simdjson/rvv/rvv_config.h"

// 2. Include RISC-V Intrinsics (if enabled)
#if defined(__riscv_vector)
  #include <riscv_vector.h>
#endif

// Helper to stringify macros for display
#define XSTR(x) STR(x)
#define STR(x) #x

void print_row(const std::string& key, const std::string& value) {
  std::cout << std::left << std::setw(25) << key << ": " << value << std::endl;
}

int main() {
  std::cout << "==================================================" << std::endl;
  std::cout << "   RVV Capabilities & Environment Report          " << std::endl;
  std::cout << "==================================================" << std::endl;

  // -------------------------------------------------------------------------
  // 1. Toolchain & Build Flags
  // -------------------------------------------------------------------------
  std::cout << "\n[Build Configuration]" << std::endl;

#ifdef __VERSION__
  print_row("Compiler Version", __VERSION__);
#endif

  // Arch detection
#if defined(__riscv) && (__riscv_xlen == 64)
  print_row("Target Architecture", "riscv64");
#else
  print_row("Target Architecture", "Unknown / Non-RV64");
#endif

  // Vector Extension detection
#ifdef __riscv_vector
  print_row("Compiler RVV Support", "Enabled (__riscv_vector)");
  print_row("Intrinsic API", "RVV 1.0 (Assumed)");
#else
  print_row("Compiler RVV Support", "DISABLED");
  print_row("Note", "Recompile with -march=rv64gcv");
#endif

  // Simdjson Logic
  print_row("SIMDJSON_IS_RVV", 
#if SIMDJSON_IS_RVV
    "1 (Active)"
#else
    "0 (Inactive)"
#endif
  );

  // -------------------------------------------------------------------------
  // 2. RVV Policy Locks (from rvv_config.h)
  // -------------------------------------------------------------------------
  std::cout << "\n[Policy Locks (rvv_config.h)]" << std::endl;
  
  print_row("RVV_BLOCK_BYTES", std::to_string(simdjson::rvv::RVV_BLOCK_BYTES));
  
#ifdef SIMDJSON_RVV_LMUL_M1
  print_row("LMUL Policy", "m1 (Fixed)");
#else
  print_row("LMUL Policy", "Unknown (Drift Detected!)");
#endif

#ifdef SIMDJSON_RVV_TAIL_SAFETY_STRICT
  print_row("Tail Safety", "STRICT");
#else
  print_row("Tail Safety", "Unknown (Drift Detected!)");
#endif

  // -------------------------------------------------------------------------
  // 3. Runtime Hardware Probing
  // -------------------------------------------------------------------------
  std::cout << "\n[Runtime Capabilities (Hardware/QEMU)]" << std::endl;

#if defined(__riscv_vector)
  // Probe VLEN using the m1 setting mandated by our spec.
  // We request the maximum AVL (-1) to get the hardware limit for 8-bit elements.
  size_t vl_max = __riscv_vsetvl_e8m1(static_cast<size_t>(-1));
  size_t vlen_bits = vl_max * 8;
  size_t vlen_bytes = vl_max;

  print_row("Runtime Instruction", "vsetvl_e8m1(AVL=-1)");
  print_row("VLMAX (elements)", std::to_string(vl_max));
  print_row("VLEN (bits)", std::to_string(vlen_bits));
  print_row("VLEN (bytes)", std::to_string(vlen_bytes));

  // Sanity Checks
  bool vlen_ok = (vlen_bits >= 128);
  print_row("VLEN Check", vlen_ok ? "PASS (>= 128)" : "WARNING (Low VLEN)");

  bool block_fit = (vlen_bytes >= simdjson::rvv::RVV_BLOCK_BYTES);
  print_row("Single-Pass Block", block_fit ? "YES (Hardware >= 64B)" : "NO (Looping required)");

#else
  std::cout << "  >> CRITICAL: Cannot probe runtime. Vector intrinsics missing." << std::endl;
#endif

  std::cout << "--------------------------------------------------" << std::endl;
  return 0;
}