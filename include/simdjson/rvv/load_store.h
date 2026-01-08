#ifndef SIMDJSON_RVV_SIMD_H
#define SIMDJSON_RVV_SIMD_H

#include "simdjson/rvv/intrinsics.h"
#include "simdjson/rvv/bitmask.h"
#include "simdjson/rvv/rvv_config.h"

#include <cstddef>   // size_t
#include <cstdint>   // uint8_t, uint64_t
#include <cstring>   // memcpy

namespace simdjson {
namespace rvv {

#if SIMDJSON_IS_RVV

template <typename T>
struct simd8x64;

// -----------------------------------------------------------------------------
// simd8x64<bool>
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
// simd8x64<uint8_t>
// -----------------------------------------------------------------------------
// Backed by a raw array because C++ structs cannot hold scalable vector types.
// RVV policy: LMUL=m1 only (VLEN-agnostic loop).
// -----------------------------------------------------------------------------
template <>
struct simd8x64<uint8_t> {
  alignas(64) uint8_t val[RVV_BLOCK_BYTES];

  simdjson_inline simd8x64() = default;

  simdjson_inline explicit simd8x64(uint8_t v) {
    for (size_t i = 0; i < RVV_BLOCK_BYTES; i++) { val[i] = v; }
  }

  simdjson_inline explicit simd8x64(const uint8_t *ptr) {
    std::memcpy(val, ptr, RVV_BLOCK_BYTES);
  }

  simdjson_inline void store(uint8_t *ptr) const {
    std::memcpy(ptr, val, RVV_BLOCK_BYTES);
  }

  simdjson_inline simd8x64<bool> operator==(uint8_t other) const {
    uint64_t full = 0;

    for (size_t i = 0; i < RVV_BLOCK_BYTES;) {
      const size_t vl = setvl_e8m1(RVV_BLOCK_BYTES - i);
      const vuint8m1_t v = __riscv_vle8_v_u8m1(val + i, vl);
      const vbool8_t m = __riscv_vmseq_vx_u8m1_b8(v, other, vl);

      uint64_t chunk = to_bitmask(m, vl);
      if (vl < 64) { chunk &= ((uint64_t(1) << vl) - 1); } // vl <= 64 by contract

      full |= (chunk << i);
      i += vl;
    }

    return simd8x64<bool>(full);
  }

  simdjson_inline simd8x64<bool> operator<=(uint8_t other) const {
    uint64_t full = 0;

    for (size_t i = 0; i < RVV_BLOCK_BYTES;) {
      const size_t vl = setvl_e8m1(RVV_BLOCK_BYTES - i);
      const vuint8m1_t v = __riscv_vle8_v_u8m1(val + i, vl);
      const vbool8_t m = __riscv_vmsleu_vx_u8m1_b8(v, other, vl);

      uint64_t chunk = to_bitmask(m, vl);
      if (vl < 64) { chunk &= ((uint64_t(1) << vl) - 1); }

      full |= (chunk << i);
      i += vl;
    }

    return simd8x64<bool>(full);
  }
};

#endif // SIMDJSON_IS_RVV

} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_SIMD_H
