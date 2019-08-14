#ifndef SIMDJSON_SIMD_INPUT_WESTMERE_H
#define SIMDJSON_SIMD_INPUT_WESTMERE_H

#include "simdjson/simd_input.h"

#ifdef IS_X86_64

TARGET_WESTMERE
namespace simdjson {

template <>
struct simd_input<Architecture::WESTMERE> {
  __m128i v0;
  __m128i v1;
  __m128i v2;
  __m128i v3;
};

template <>
really_inline simd_input<Architecture::WESTMERE>
fill_input<Architecture::WESTMERE>(const uint8_t *ptr) {
  struct simd_input<Architecture::WESTMERE> in;
  in.v0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 0));
  in.v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 16));
  in.v2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 32));
  in.v3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 48));
  return in;
}

template <>
really_inline uint64_t cmp_mask_against_input<Architecture::WESTMERE>(
    simd_input<Architecture::WESTMERE> in, uint8_t m) {
  const __m128i mask = _mm_set1_epi8(m);
  __m128i cmp_res_0 = _mm_cmpeq_epi8(in.v0, mask);
  uint64_t res_0 = _mm_movemask_epi8(cmp_res_0);
  __m128i cmp_res_1 = _mm_cmpeq_epi8(in.v1, mask);
  uint64_t res_1 = _mm_movemask_epi8(cmp_res_1);
  __m128i cmp_res_2 = _mm_cmpeq_epi8(in.v2, mask);
  uint64_t res_2 = _mm_movemask_epi8(cmp_res_2);
  __m128i cmp_res_3 = _mm_cmpeq_epi8(in.v3, mask);
  uint64_t res_3 = _mm_movemask_epi8(cmp_res_3);
  return res_0 | (res_1 << 16) | (res_2 << 32) | (res_3 << 48);
}

template <>
really_inline uint64_t unsigned_lteq_against_input<Architecture::WESTMERE>(
    simd_input<Architecture::WESTMERE> in, uint8_t m) {
  const __m128i maxval = _mm_set1_epi8(m);
  __m128i cmp_res_0 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, in.v0), maxval);
  uint64_t res_0 = _mm_movemask_epi8(cmp_res_0);
  __m128i cmp_res_1 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, in.v1), maxval);
  uint64_t res_1 = _mm_movemask_epi8(cmp_res_1);
  __m128i cmp_res_2 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, in.v2), maxval);
  uint64_t res_2 = _mm_movemask_epi8(cmp_res_2);
  __m128i cmp_res_3 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, in.v3), maxval);
  uint64_t res_3 = _mm_movemask_epi8(cmp_res_3);
  return res_0 | (res_1 << 16) | (res_2 << 32) | (res_3 << 48);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_SIMD_INPUT_WESTMERE_H
