#ifndef SIMDJSON_STAGE1_FIND_MARKS_WESTMERE_H
#define SIMDJSON_STAGE1_FIND_MARKS_WESTMERE_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "westmere/simd_input.h"
#include "westmere/simdutf8check.h"
#include "simdjson/stage1_find_marks.h"

namespace simdjson {

TARGET_WESTMERE
namespace westmere {

static const Architecture ARCHITECTURE = Architecture::WESTMERE;

static really_inline uint64_t compute_quote_mask(uint64_t quote_bits) {
  return _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFFu), 0));
}

static really_inline void find_whitespace_and_structurals(simd_input<ARCHITECTURE> in,
  uint64_t &whitespace, uint64_t &structurals) {

  const __m128i structural_table =
      _mm_setr_epi8(44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123);
  const __m128i white_table = _mm_setr_epi8(32, 100, 100, 100, 17, 100, 113, 2,
                                              100, 9, 10, 112, 100, 13, 100, 100);
  const __m128i struct_offset = _mm_set1_epi8(0xd4u);
  const __m128i struct_mask = _mm_set1_epi8(32);

  __m128i white0 = _mm_cmpeq_epi8(in.v0, _mm_shuffle_epi8(white_table, in.v0));
  __m128i white1 = _mm_cmpeq_epi8(in.v1, _mm_shuffle_epi8(white_table, in.v1));
  __m128i white2 = _mm_cmpeq_epi8(in.v2, _mm_shuffle_epi8(white_table, in.v2));
  __m128i white3 = _mm_cmpeq_epi8(in.v3, _mm_shuffle_epi8(white_table, in.v3));
  uint64_t ws_res_0 = _mm_movemask_epi8(white0);
  uint64_t ws_res_1 = _mm_movemask_epi8(white1);
  uint64_t ws_res_2 = _mm_movemask_epi8(white2);
  uint64_t ws_res_3 = _mm_movemask_epi8(white3);

  whitespace =
      (ws_res_0 | (ws_res_1 << 16) | (ws_res_2 << 32) | (ws_res_3 << 48));

  __m128i struct1_r1 = _mm_add_epi8(struct_offset, in.v0);
  __m128i struct2_r1 = _mm_add_epi8(struct_offset, in.v1);
  __m128i struct3_r1 = _mm_add_epi8(struct_offset, in.v2);
  __m128i struct4_r1 = _mm_add_epi8(struct_offset, in.v3);

  __m128i struct1_r2 = _mm_or_si128(in.v0, struct_mask);
  __m128i struct2_r2 = _mm_or_si128(in.v1, struct_mask);
  __m128i struct3_r2 = _mm_or_si128(in.v2, struct_mask);
  __m128i struct4_r2 = _mm_or_si128(in.v3, struct_mask);

  __m128i struct1_r3 = _mm_shuffle_epi8(structural_table, struct1_r1);
  __m128i struct2_r3 = _mm_shuffle_epi8(structural_table, struct2_r1);
  __m128i struct3_r3 = _mm_shuffle_epi8(structural_table, struct3_r1);
  __m128i struct4_r3 = _mm_shuffle_epi8(structural_table, struct4_r1);

  __m128i struct1 = _mm_cmpeq_epi8(struct1_r2, struct1_r3);
  __m128i struct2 = _mm_cmpeq_epi8(struct2_r2, struct2_r3);
  __m128i struct3 = _mm_cmpeq_epi8(struct3_r2, struct3_r3);
  __m128i struct4 = _mm_cmpeq_epi8(struct4_r2, struct4_r3);

  uint64_t structural_res_0 = _mm_movemask_epi8(struct1);
  uint64_t structural_res_1 = _mm_movemask_epi8(struct2);
  uint64_t structural_res_2 = _mm_movemask_epi8(struct3);
  uint64_t structural_res_3 = _mm_movemask_epi8(struct4);

  structurals = (structural_res_0 | (structural_res_1 << 16) |
                  (structural_res_2 << 32) | (structural_res_3 << 48));
}

#include "generic/stage1_find_marks_flatten.h"
#include "generic/stage1_find_marks.h"

} // namespace westmere
UNTARGET_REGION

template <>
int find_structural_bits<Architecture::WESTMERE>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj) {
  return westmere::find_structural_bits(buf, len, pj);
}

} // namespace simdjson

#endif // IS_X86_64
#endif // SIMDJSON_STAGE1_FIND_MARKS_WESTMERE_H
