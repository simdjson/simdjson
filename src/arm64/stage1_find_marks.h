#ifndef SIMDJSON_ARM64_STAGE1_FIND_MARKS_H
#define SIMDJSON_ARM64_STAGE1_FIND_MARKS_H

#include "simdjson/portability.h"

#ifdef IS_ARM64

#include "arm64/architecture.h"
#include "arm64/simd_input.h"
#include "arm64/simdutf8check.h"
#include "simdjson/stage1_find_marks.h"

namespace simdjson::arm64 {

really_inline uint64_t compute_quote_mask(uint64_t quote_bits) {

#ifdef __ARM_FEATURE_CRYPTO // some ARM processors lack this extension
  return vmull_p64(-1ULL, quote_bits);
#else
  return portable_compute_quote_mask(quote_bits);
#endif
}

really_inline void find_whitespace_and_structurals(
    simd_input<ARCHITECTURE> in, uint64_t &whitespace,
    uint64_t &structurals) {
  const uint8x16_t low_nibble_mask =
      (uint8x16_t){16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0};
  const uint8x16_t high_nibble_mask =
      (uint8x16_t){8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0};
  const uint8x16_t low_nib_and_mask = vmovq_n_u8(0xf);

  auto v = in.map([&](auto chunk) {
    uint8x16_t nib_lo = vandq_u8(chunk, low_nib_and_mask);
    uint8x16_t nib_hi = vshrq_n_u8(chunk, 4);
    uint8x16_t shuf_lo = vqtbl1q_u8(low_nibble_mask, nib_lo);
    uint8x16_t shuf_hi = vqtbl1q_u8(high_nibble_mask, nib_hi);
    return vandq_u8(shuf_lo, shuf_hi);
  });

  const uint8x16_t structural_shufti_mask = vmovq_n_u8(0x7);
  structurals = MAP_BITMASK( v, vtstq_u8(_v, structural_shufti_mask) );

  const uint8x16_t whitespace_shufti_mask = vmovq_n_u8(0x18);
  whitespace = MAP_BITMASK( v, vtstq_u8(_v, whitespace_shufti_mask) );
}

#include "generic/stage1_find_marks_flatten.h"
#include "generic/stage1_find_marks.h"

} // namespace simdjson::arm64

namespace simdjson {

template <>
int find_structural_bits<Architecture::ARM64>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj) {
  return arm64::find_structural_bits(buf, len, pj);
}

} // namespace simdjson

#endif // IS_ARM64
#endif // SIMDJSON_ARM64_STAGE1_FIND_MARKS_H
