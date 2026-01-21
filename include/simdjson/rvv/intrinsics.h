#ifndef SIMDJSON_RVV_INTRINSICS_H
#define SIMDJSON_RVV_INTRINSICS_H

#include "simdjson/base.h"

// -----------------------------------------------------------------------------
// RVV intrinsics wrapper (RVV 1.0, explicit intrinsics)
// -----------------------------------------------------------------------------
// Requirements:
// - Must NOT break non-RVV builds when included indirectly.
// - Must NOT redefine standard RVV typedef names from <riscv_vector.h>.
// - Keep RVV-only helpers inside simdjson::rvv.
// - Locked decisions: LMUL=m1, semantic block size = 64 bytes.
// -----------------------------------------------------------------------------

#if SIMDJSON_IS_RVV

#include <riscv_vector.h>

namespace simdjson {
namespace rvv {

// -----------------------------------------------------------------------------
// Fixed configuration (per spec)
// -----------------------------------------------------------------------------
static constexpr size_t RVV_BLOCK_BYTES = 64;

// Optional marker macro (internal). Prefer constexpr in code.
#ifndef SIMDJSON_RVV_LMUL_1
#define SIMDJSON_RVV_LMUL_1 1
#endif

// -----------------------------------------------------------------------------
// Helpers: vsetvl
// -----------------------------------------------------------------------------
simdjson_inline size_t setvl_e8m1(size_t remaining) {
  return __riscv_vsetvl_e8m1(remaining);
}

// -----------------------------------------------------------------------------
// Helpers: reinterprets (typed, no-cost)
// -----------------------------------------------------------------------------
simdjson_inline vint8m1_t reinterpret_u8_as_i8(vuint8m1_t v) {
  return __riscv_vreinterpret_v_u8m1_i8m1(v);
}

simdjson_inline vuint8m1_t reinterpret_i8_as_u8(vint8m1_t v) {
  return __riscv_vreinterpret_v_i8m1_u8m1(v);
}

} // namespace rvv
} // namespace simdjson

#else  // !SIMDJSON_IS_RVV

// Non-RVV build:
// Do not include <riscv_vector.h>, do not declare RVV types.
// This header remains safely includable on any target.

#endif // SIMDJSON_IS_RVV

#endif // SIMDJSON_RVV_INTRINSICS_H
