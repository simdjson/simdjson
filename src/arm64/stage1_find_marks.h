#ifndef SIMDJSON_ARM64_STAGE1_FIND_MARKS_H
#define SIMDJSON_ARM64_STAGE1_FIND_MARKS_H

#include "simdjson.h"
#include "arm64/bitmask.h"
#include "arm64/simd.h"
#include "arm64/bitmanipulation.h"
#include "arm64/implementation.h"

namespace simdjson::arm64 {

using namespace simd;

struct json_character_block {
  static really_inline json_character_block classify(const simd::simd8x64<uint8_t> in);

  really_inline uint64_t whitespace() const { return _whitespace; }
  really_inline uint64_t op() const { return _op; }
  really_inline uint64_t scalar() { return ~(op() | whitespace()); }

  uint64_t _whitespace;
  uint64_t _op;
};

really_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t> in) {
  auto v = in.map<uint8_t>([&](simd8<uint8_t> chunk) {
    auto nib_lo = chunk & 0xf;
    auto nib_hi = chunk.shr<4>();
    auto shuf_lo = nib_lo.lookup_16<uint8_t>(16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
    auto shuf_hi = nib_hi.lookup_16<uint8_t>(8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0);
    return shuf_lo & shuf_hi;
  });


  // We compute whitespace and op separately. If the code later only use one or the
  // other, given the fact that all functions are aggressively inlined, we can
  // hope that useless computations will be omitted. This is namely case when
  // minifying (we only need whitespace). *However* if we only need spaces,
  // it is likely that we will still compute 'v' above with two lookup_16: one
  // could do it a bit cheaper. This is in contrast with the x64 implementations
  // where we can, efficiently, do the white space and structural matching
  // separately. One reason for this difference is that on ARM NEON, the table
  // lookups either zero or leave unchanged the characters exceeding 0xF whereas
  // on x64, the equivalent instruction (pshufb) automatically applies a mask,
  // ignoring the 4 most significant bits. Thus the x64 implementation is
  // optimized differently. This being said, if you use this code strictly
  // just for minification (or just to identify the structural characters),
  // there is a small untaken optimization opportunity here. We deliberately
  // do not pick it up.

  uint64_t op = v.map([&](simd8<uint8_t> _v) { return _v.any_bits_set(0x7); }).to_bitmask();
  uint64_t whitespace = v.map([&](simd8<uint8_t> _v) { return _v.any_bits_set(0x18); }).to_bitmask();
  return { whitespace, op };
}

really_inline bool is_ascii(simd8x64<uint8_t> input) {
    simd8<uint8_t> bits = input.reduce([&](auto a,auto b) { return a|b; });
    return bits.max() < 0b10000000u;
}

really_inline simd8<bool> must_be_continuation(simd8<uint8_t> prev1, simd8<uint8_t> prev2, simd8<uint8_t> prev3) {
    simd8<bool> is_second_byte = prev1 >= uint8_t(0b11000000u);
    simd8<bool> is_third_byte  = prev2 >= uint8_t(0b11100000u);
    simd8<bool> is_fourth_byte = prev3 >= uint8_t(0b11110000u);
    // Use ^ instead of | for is_*_byte, because ^ is commutative, and the caller is using ^ as well.
    // This will work fine because we only have to report errors for cases with 0-1 lead bytes.
    // Multiple lead bytes implies 2 overlapping multibyte characters, and if that happens, there is
    // guaranteed to be at least *one* lead byte that is part of only 1 other multibyte character.
    // The error will be detected there.
    return is_second_byte ^ is_third_byte ^ is_fourth_byte;
}

#include "generic/buf_block_reader.h"
#include "generic/json_string_scanner.h"
#include "generic/json_scanner.h"

#include "generic/json_minifier.h"
WARN_UNUSED error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return arm64::stage1::json_minifier::minify<64>(buf, len, dst, dst_len);
}

#include "generic/utf8_lookup2_algorithm.h"
#include "generic/json_structural_indexer.h"
WARN_UNUSED error_code implementation::stage1(const uint8_t *buf, size_t len, parser &parser, bool streaming) const noexcept {
  return arm64::stage1::json_structural_indexer::index<64>(buf, len, parser, streaming);
}

} // namespace simdjson::arm64

#endif // SIMDJSON_ARM64_STAGE1_FIND_MARKS_H
