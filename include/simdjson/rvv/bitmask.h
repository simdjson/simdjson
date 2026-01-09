#ifndef SIMDJSON_RVV_BITMASK_H
#define SIMDJSON_RVV_BITMASK_H

#include "simdjson/rvv/intrinsics.h"

namespace simdjson {
namespace rvv {

#if SIMDJSON_IS_RVV

// -----------------------------------------------------------------------------
// RVV mask -> uint64_t export (STORE_AND_PACK_U64)
// -----------------------------------------------------------------------------
// Locked architecture decision 6.2:
// - No vfirst/vcpop iteration as the primary export method.
// - Export by storing the mask to memory and packing into a uint64_t.
//
// Notes:
// - This helper is intended for byte-lane predicates (vbool8_t).
// - Valid for vl in [0, 64] (since we export into 64 bits).
// - Layout: bit 0 corresponds to element 0, little-endian bit order.
// -----------------------------------------------------------------------------

simdjson_inline uint64_t to_bitmask(vbool8_t mask, size_t vl) {
  // Defensive: callers should respect vl <= 64.
  // If violated, we clamp to 64 to avoid shifting/packing UB downstream.
  if (vl > 64) { vl = 64; }

  // vsm writes packed bits to memory; number of bytes written is ceil(vl/8).
  // Zero-init ensures upper bytes are clean when vl < 64.
  alignas(8) uint64_t out = 0;

  // Store mask bits to memory (packed).
  // Signature expects a byte pointer; char/uint8_t aliasing is allowed.
  __riscv_vsm_v_b8(reinterpret_cast<uint8_t*>(&out), mask, vl);

  // out now contains packed bits in the low bits (bit i corresponds to lane i).
  return out;
}

#endif // SIMDJSON_IS_RVV

} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_BITMASK_H
