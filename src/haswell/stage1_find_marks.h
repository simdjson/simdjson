#ifndef SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H
#define SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "haswell/simd_input.h"
#include "haswell/simdutf8check.h"
#include "simdjson/stage1_find_marks.h"

TARGET_HASWELL
namespace simdjson::haswell {

really_inline uint64_t compute_quote_mask(const uint64_t quote_bits) {
  // There should be no such thing with a processing supporting avx2
  // but not clmul.
  uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFFu), 0));
  return quote_mask;
}

really_inline void find_whitespace_and_operators(
  const simd_input in,
  uint64_t &whitespace, uint64_t &op) {

  #ifdef SIMDJSON_NAIVE_STRUCTURAL

    // You should never need this naive approach, but it can be useful
    // for research purposes
    const __m256i mask_open_brace = _mm256_set1_epi8(0x7b);
    const __m256i mask_close_brace = _mm256_set1_epi8(0x7d);
    const __m256i mask_open_bracket = _mm256_set1_epi8(0x5b);
    const __m256i mask_close_bracket = _mm256_set1_epi8(0x5d);
    const __m256i mask_column = _mm256_set1_epi8(0x3a);
    const __m256i mask_comma = _mm256_set1_epi8(0x2c);
    op = in.map([&](auto in) {
      __m256i op = _mm256_cmpeq_epi8(in, mask_open_brace);
      op = _mm256_or_si256(op, _mm256_cmpeq_epi8(in, mask_close_brace));
      op = _mm256_or_si256(op, _mm256_cmpeq_epi8(in, mask_open_bracket));
      op = _mm256_or_si256(op, _mm256_cmpeq_epi8(in, mask_close_bracket));
      op = _mm256_or_si256(op, _mm256_cmpeq_epi8(in, mask_column));
      op = _mm256_or_si256(op, _mm256_cmpeq_epi8(in, mask_comma));
      return op;
    }).to_bitmask();

    const __m256i mask_space = _mm256_set1_epi8(0x20);
    const __m256i mask_linefeed = _mm256_set1_epi8(0x0a);
    const __m256i mask_tab = _mm256_set1_epi8(0x09);
    const __m256i mask_carriage = _mm256_set1_epi8(0x0d);
    whitespace = in.map([&](auto in) {
      __m256i space = _mm256_cmpeq_epi8(in, mask_space);
      space = _mm256_or_si256(space, _mm256_cmpeq_epi8(in, mask_linefeed));
      space = _mm256_or_si256(space, _mm256_cmpeq_epi8(in, mask_tab));
      space = _mm256_or_si256(space, _mm256_cmpeq_epi8(in, mask_carriage));
      return space;
    }).to_bitmask();
    // end of naive approach

  #else  // SIMDJSON_NAIVE_STRUCTURAL

    // clang-format off
    const __m256i operator_table =
        _mm256_setr_epi8(44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123,
                         44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123);
    const __m256i white_table = _mm256_setr_epi8(
        32, 100, 100, 100, 17, 100, 113, 2, 100, 9, 10, 112, 100, 13, 100, 100,
        32, 100, 100, 100, 17, 100, 113, 2, 100, 9, 10, 112, 100, 13, 100, 100);
    // clang-format on
    const __m256i op_offset = _mm256_set1_epi8(0xd4u);
    const __m256i op_mask = _mm256_set1_epi8(32);

    whitespace = in.map([&](auto _in) {
      return _mm256_cmpeq_epi8(_in, _mm256_shuffle_epi8(white_table, _in));
    }).to_bitmask();

    op = in.map([&](auto _in) {
      const __m256i r1 = _mm256_add_epi8(op_offset, _in);
      const __m256i r2 = _mm256_or_si256(_in, op_mask);
      const __m256i r3 = _mm256_shuffle_epi8(operator_table, r1);
      return _mm256_cmpeq_epi8(r2, r3);
    }).to_bitmask();

  #endif // else SIMDJSON_NAIVE_STRUCTURAL
}

#include "generic/stage1_find_marks.h"

} // namespace haswell
UNTARGET_REGION

TARGET_HASWELL
namespace simdjson {

template <>
int find_structural_bits<Architecture::HASWELL>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming) {
  return haswell::find_structural_bits(buf, len, pj, streaming);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H
