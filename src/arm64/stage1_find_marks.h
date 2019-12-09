#ifndef SIMDJSON_ARM64_STAGE1_FIND_MARKS_H
#define SIMDJSON_ARM64_STAGE1_FIND_MARKS_H

#include "simdjson/portability.h"

#ifdef IS_ARM64

#include "arm64/bitmask.h"
#include "arm64/simd.h"
#include "simdjson/stage1_find_marks.h"

namespace simdjson::arm64 {

using namespace simd;

really_inline void find_whitespace_and_operators(
  const simd::simd8x64<uint8_t> in,
  uint64_t &whitespace, uint64_t &op) {

  auto v = in.map<uint8_t>([&](simd8<uint8_t> chunk) {
    auto nib_lo = chunk & 0xf;
    auto nib_hi = chunk.shr<4>();
    auto shuf_lo = nib_lo.lookup_16<uint8_t>(16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
    auto shuf_hi = nib_hi.lookup_16<uint8_t>(8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0);
    return shuf_lo & shuf_hi;
  });

  op = v.map([&](simd8<uint8_t> _v) { return _v.any_bits_set(0x7); }).to_bitmask();
  whitespace = v.map([&](simd8<uint8_t> _v) { return _v.any_bits_set(0x18); }).to_bitmask();
}

#include "generic/utf8_lookup_algorithm.h"
#include "generic/stage1_find_marks.h"

} // namespace simdjson::arm64

namespace simdjson {

template <>
int find_structural_bits<Architecture::ARM64>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming) {
  return arm64::stage1::find_structural_bits<64>(buf, len, pj, streaming);
}

} // namespace simdjson

#endif // IS_ARM64
#endif // SIMDJSON_ARM64_STAGE1_FIND_MARKS_H
