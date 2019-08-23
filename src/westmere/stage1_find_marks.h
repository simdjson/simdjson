#ifndef SIMDJSON_WESTMERE_STAGE1_FIND_MARKS_H
#define SIMDJSON_WESTMERE_STAGE1_FIND_MARKS_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "westmere/architecture.h"
#include "westmere/simd_input.h"
#include "westmere/simdutf8check.h"
#include "simdjson/stage1_find_marks.h"

TARGET_WESTMERE
namespace simdjson::westmere {

really_inline uint64_t compute_quote_mask(uint64_t quote_bits) {
  return _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFFu), 0));
}

really_inline void find_whitespace_and_structurals(simd_input<ARCHITECTURE> in,
  uint64_t &whitespace, uint64_t &structurals) {

  const __m128i structural_table =
      _mm_setr_epi8(44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123);
  const __m128i white_table = _mm_setr_epi8(32, 100, 100, 100, 17, 100, 113, 2,
                                              100, 9, 10, 112, 100, 13, 100, 100);
  const __m128i struct_offset = _mm_set1_epi8(0xd4u);
  const __m128i struct_mask = _mm_set1_epi8(32);

  whitespace = in.MAP_BITMASK( _mm_cmpeq_epi8(chunk, _mm_shuffle_epi8(white_table, chunk)) );

  auto r1 = in.MAP_CHUNKS( _mm_add_epi8(struct_offset, chunk) );
  auto r2 = in.MAP_CHUNKS( _mm_or_si128(chunk, struct_mask) );
  auto r3 = r1.MAP_CHUNKS( _mm_shuffle_epi8(structural_table, chunk) );
  structurals = simd_input<ARCHITECTURE>(
    _mm_cmpeq_epi8(r2.v0, r3.v0),
    _mm_cmpeq_epi8(r2.v1, r3.v1),
    _mm_cmpeq_epi8(r2.v2, r3.v2),
    _mm_cmpeq_epi8(r2.v3, r3.v3)
  ).to_bitmask();
}

#include "generic/stage1_find_marks_flatten.h"
#include "generic/stage1_find_marks.h"

} // namespace westmere
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {

template <>
int find_structural_bits<Architecture::WESTMERE>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj) {
  return westmere::find_structural_bits(buf, len, pj);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_WESTMERE_STAGE1_FIND_MARKS_H
