#ifndef SIMDJSON_RVV_SIMD_H
#define SIMDJSON_RVV_SIMD_H

#include "simdjson/rvv/intrinsics.h"
#include "simdjson/rvv/bitmask.h"

#include <cstring>   // memcpy
#include <cstdint>   // uint8_t, uint64_t
#include <type_traits>

namespace simdjson {
namespace rvv {

#if SIMDJSON_IS_RVV

// Forward decl (kept for parity with other backends)
template <typename T>
struct simd8x64;

// -----------------------------------------------------------------------------
// simd8x64<bool>
// -----------------------------------------------------------------------------
// Holds a 64-lane predicate as a scalar uint64_t bitmask.
// Bit i corresponds to lane i (0..63).
// -----------------------------------------------------------------------------
template <>
struct simd8x64<bool> {
  uint64_t val{0};

  simdjson_inline simd8x64() = default;
  simdjson_inline explicit simd8x64(uint64_t v) : val(v) {}

  simdjson_inline uint64_t to_bitmask() const { return val; }

  simdjson_inline simd8x64 operator&(const simd8x64 &other) const { return simd8x64(val & other.val); }
  simdjson_inline simd8x64 operator|(const simd8x64 &other) const { return simd8x64(val | other.val); }
  simdjson_inline simd8x64 operator~() const { return simd8x64(~val); }
};

// -----------------------------------------------------------------------------
// simd8x64<uint8_t> (byte block)
// -----------------------------------------------------------------------------
// This is the only specialization we expose for stage1 byte classification.
// The generic template is intentionally not provided to avoid accidental misuse.
// -----------------------------------------------------------------------------
template <>
struct simd8x64<uint8_t> {
  alignas(64) uint8_t val[64];

  simdjson_inline simd8x64() = default;

  simdjson_inline explicit simd8x64(uint8_t v) {
    // scalar fill is fine; compilers typically unroll / vectorize
    for (size_t i = 0; i < 64; i++) { val[i] = v; }
  }

  simdjson_inline explicit simd8x64(const uint8_t *ptr) {
    std::memcpy(val, ptr, sizeof(val));
  }

  simdjson_inline void store(uint8_t *ptr) const {
    std::memcpy(ptr, val, sizeof(val));
  }

  // Equality compare against an immediate byte.
  simdjson_inline simd8x64<bool> operator==(uint8_t other) const {
    uint64_t full = 0;
    size_t vl = 0;
    for (size_t i = 0; i < 64; i += vl) {
      vl = setvl_e8m1(64 - i);
      vuint8m1_t v = __riscv_vle8_v_u8m1(val + i, vl);
      vbool8_t m = __riscv_vmseq_vx_u8m1_b8(v, other, vl);
      full |= (to_bitmask(m, vl) << i);
    }
    return simd8x64<bool>(full);
  }

  // Unsigned <= compare against an immediate byte.
  simdjson_inline simd8x64<bool> operator<=(uint8_t other) const {
    uint64_t full = 0;
    size_t vl = 0;
    for (size_t i = 0; i < 64; i += vl) {
      vl = setvl_e8m1(64 - i);
      vuint8m1_t v = __riscv_vle8_v_u8m1(val + i, vl);
      // Unsigned compare (important!)
      vbool8_t m = __riscv_vmsleu_vx_u8m1_b8(v, other, vl);
      full |= (to_bitmask(m, vl) << i);
    }
    return simd8x64<bool>(full);
  }
};

#endif // SIMDJSON_IS_RVV

} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_SIMD_H
