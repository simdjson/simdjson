#ifndef SIMDJSON_ARM64_SIMD_INPUT_H
#define SIMDJSON_ARM64_SIMD_INPUT_H

#include "simdjson/common_defs.h"
#include "simdjson/portability.h"
#include "simdjson/simdjson.h"

#ifdef IS_ARM64

namespace simdjson::arm64 {

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

struct simd_input {
  const uint8x16_t chunks[4];

  really_inline simd_input()
    : chunks{uint8x16_t(), uint8x16_t(), uint8x16_t(), uint8x16_t() } {}

  really_inline simd_input(const uint8x16_t chunk0, const uint8x16_t chunk1, const uint8x16_t chunk2, const uint8x16_t chunk3)
    : chunks{chunk0, chunk1, chunk2, chunk3 } {}

  really_inline simd_input(const uint8_t *ptr)
      : chunks{
        vld1q_u8(ptr + 0*16),
        vld1q_u8(ptr + 1*16),
        vld1q_u8(ptr + 2*16),
        vld1q_u8(ptr + 3*16)
       } {}

  template <typename F>
  really_inline void each(F const& each_chunk) const {
    each_chunk(this->chunks[0]);
    each_chunk(this->chunks[1]);
    each_chunk(this->chunks[2]);
    each_chunk(this->chunks[3]);
  }

  template <typename F>
  really_inline simd_input map(F const& map_chunk) const {
    return simd_input(
      map_chunk(this->chunks[0]),
      map_chunk(this->chunks[1]),
      map_chunk(this->chunks[2]),
      map_chunk(this->chunks[3])
    );
  }

  template <typename F>
  really_inline simd_input map(simd_input b, F const& map_chunk) const {
    return simd_input(
      map_chunk(this->chunks[0], b.chunks[0]),
      map_chunk(this->chunks[1], b.chunks[1]),
      map_chunk(this->chunks[2], b.chunks[2]),
      map_chunk(this->chunks[3], b.chunks[3])
    );
  }

  template <typename F>
  really_inline uint8x16_t reduce(F const& reduce_pair) const {
    uint8x16_t r01 = reduce_pair(this->chunks[0], this->chunks[1]);
    uint8x16_t r23 = reduce_pair(this->chunks[2], this->chunks[3]);
    return reduce_pair(r01, r23);
  }

  really_inline uint64_t to_bitmask() const {
    return neon_movemask_bulk(this->chunks[0], this->chunks[1], this->chunks[2], this->chunks[3]);
  }

  really_inline simd_input bit_or(const uint8_t m) const {
    const uint8x16_t mask = vmovq_n_u8(m);
    return this->map( [&](auto a) {
      return vorrq_u8(a, mask);
    });
  }

  really_inline uint64_t eq(const uint8_t m) const {
    const uint8x16_t mask = vmovq_n_u8(m);
    return this->map( [&](auto a) {
      return vceqq_u8(a, mask);
    }).to_bitmask();
  }

  really_inline uint64_t lteq(const uint8_t m) const {
    const uint8x16_t mask = vmovq_n_u8(m);
    return this->map( [&](auto a) {
      return vcleq_u8(a, mask);
    }).to_bitmask();
  }

}; // struct simd_input

} // namespace simdjson::arm64

#endif // IS_ARM64
#endif // SIMDJSON_ARM64_SIMD_INPUT_H
