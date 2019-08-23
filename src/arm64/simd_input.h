#ifndef SIMDJSON_ARM64_SIMD_INPUT_H
#define SIMDJSON_ARM64_SIMD_INPUT_H

#include "../simd_input.h"

#ifdef IS_ARM64

namespace simdjson {

really_inline uint16_t neon_movemask(uint8x16_t input) {
  const uint8x16_t bit_mask = {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                              0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  uint8x16_t minput = vandq_u8(input, bit_mask);
  uint8x16_t tmp = vpaddq_u8(minput, minput);
  tmp = vpaddq_u8(tmp, tmp);
  tmp = vpaddq_u8(tmp, tmp);
  return vgetq_lane_u16(vreinterpretq_u16_u8(tmp), 0);
}

really_inline uint64_t neon_movemask_bulk(uint8x16_t p0, uint8x16_t p1,
                                          uint8x16_t p2, uint8x16_t p3) {
  const uint8x16_t bit_mask = {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                              0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  uint8x16_t t0 = vandq_u8(p0, bit_mask);
  uint8x16_t t1 = vandq_u8(p1, bit_mask);
  uint8x16_t t2 = vandq_u8(p2, bit_mask);
  uint8x16_t t3 = vandq_u8(p3, bit_mask);
  uint8x16_t sum0 = vpaddq_u8(t0, t1);
  uint8x16_t sum1 = vpaddq_u8(t2, t3);
  sum0 = vpaddq_u8(sum0, sum1);
  sum0 = vpaddq_u8(sum0, sum0);
  return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
}

template <>
struct simd_input<Architecture::ARM64> {
  uint8x16_t i0;
  uint8x16_t i1;
  uint8x16_t i2;
  uint8x16_t i3;

  really_inline simd_input(const uint8_t *ptr) {
    this->i0 = vld1q_u8(ptr + 0);
    this->i1 = vld1q_u8(ptr + 16);
    this->i2 = vld1q_u8(ptr + 32);
    this->i3 = vld1q_u8(ptr + 48);
  }

  really_inline simd_input(uint8x16_t i0, uint8x16_t i1, uint8x16_t i2, uint8x16_t i3) {
    this->i0 = i0;
    this->i1 = i1;
    this->i2 = i2;
    this->i3 = i3;
  }

  template <typename F>
  really_inline uint64_t build_bitmask(F const& chunk_to_mask) {
    uint8x16_t r0 = chunk_to_mask(this->i0);
    uint8x16_t r1 = chunk_to_mask(this->i1);
    uint8x16_t r2 = chunk_to_mask(this->i2);
    uint8x16_t r3 = chunk_to_mask(this->i3);
    return neon_movemask_bulk(r0, r1, r2, r3);
  }

  template <typename F>
  really_inline simd_input<Architecture::ARM64> map(F const& map_chunk) {
    simd_input<Architecture::ARM64> result = {
      map_chunk(this->i0),
      map_chunk(this->i1),
      map_chunk(this->i2),
      map_chunk(this->i3)
    };
    return result;
  }

  really_inline uint64_t eq(uint8_t m) {
    const uint8x16_t mask = vmovq_n_u8(m);
    return this->build_bitmask([&](uint8x16_t chunk) {
      return vceqq_u8(chunk, mask);
    });
  }

  really_inline uint64_t lteq(uint8_t m) {
    const uint8x16_t mask = vmovq_n_u8(m);
    return this->build_bitmask([&](uint8x16_t chunk) {
      return vcleq_u8(chunk, mask);
    });
  }

}; // struct simd_input

} // namespace simdjson

#endif // IS_ARM64
#endif // SIMDJSON_ARM64_SIMD_INPUT_H
