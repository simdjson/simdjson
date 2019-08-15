#ifndef SIMDJSON_SIMD_INPUT_ARM64_H
#define SIMDJSON_SIMD_INPUT_ARM64_H

#include "simdjson/simd_input.h"

#ifdef IS_ARM64
namespace simdjson {

template <>
struct simd_input<Architecture::ARM64> {
  uint8x16_t i0;
  uint8x16_t i1;
  uint8x16_t i2;
  uint8x16_t i3;
};

template <>
really_inline simd_input<Architecture::ARM64>
fill_input<Architecture::ARM64>(const uint8_t *ptr) {
  struct simd_input<Architecture::ARM64> in;
  in.i0 = vld1q_u8(ptr + 0);
  in.i1 = vld1q_u8(ptr + 16);
  in.i2 = vld1q_u8(ptr + 32);
  in.i3 = vld1q_u8(ptr + 48);
  return in;
}

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
really_inline uint64_t cmp_mask_against_input<Architecture::ARM64>(
    simd_input<Architecture::ARM64> in, uint8_t m) {
  const uint8x16_t mask = vmovq_n_u8(m);
  uint8x16_t cmp_res_0 = vceqq_u8(in.i0, mask);
  uint8x16_t cmp_res_1 = vceqq_u8(in.i1, mask);
  uint8x16_t cmp_res_2 = vceqq_u8(in.i2, mask);
  uint8x16_t cmp_res_3 = vceqq_u8(in.i3, mask);
  return neon_movemask_bulk(cmp_res_0, cmp_res_1, cmp_res_2, cmp_res_3);
}

template <>
really_inline uint64_t unsigned_lteq_against_input<Architecture::ARM64>(
    simd_input<Architecture::ARM64> in, uint8_t m) {
  const uint8x16_t mask = vmovq_n_u8(m);
  uint8x16_t cmp_res_0 = vcleq_u8(in.i0, mask);
  uint8x16_t cmp_res_1 = vcleq_u8(in.i1, mask);
  uint8x16_t cmp_res_2 = vcleq_u8(in.i2, mask);
  uint8x16_t cmp_res_3 = vcleq_u8(in.i3, mask);
  return neon_movemask_bulk(cmp_res_0, cmp_res_1, cmp_res_2, cmp_res_3);
}

} // namespace simdjson

#endif // IS_ARM64
#endif // SIMDJSON_SIMD_INPUT_ARM64_H
