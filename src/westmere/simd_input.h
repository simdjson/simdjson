#ifndef SIMDJSON_WESTMERE_SIMD_INPUT_H
#define SIMDJSON_WESTMERE_SIMD_INPUT_H

#include "../simd_input.h"

#ifdef IS_X86_64

TARGET_WESTMERE
namespace simdjson {

template <>
struct simd_input<Architecture::WESTMERE> {
  __m128i v0;
  __m128i v1;
  __m128i v2;
  __m128i v3;

  really_inline simd_input(const uint8_t *ptr) {
    this->v0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 0));
    this->v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 16));
    this->v2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 32));
    this->v3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 48));
  }

  really_inline uint64_t eq(uint8_t m) {
    const __m128i mask = _mm_set1_epi8(m);
    __m128i cmp_res_0 = _mm_cmpeq_epi8(this->v0, mask);
    uint64_t res_0 = _mm_movemask_epi8(cmp_res_0);
    __m128i cmp_res_1 = _mm_cmpeq_epi8(this->v1, mask);
    uint64_t res_1 = _mm_movemask_epi8(cmp_res_1);
    __m128i cmp_res_2 = _mm_cmpeq_epi8(this->v2, mask);
    uint64_t res_2 = _mm_movemask_epi8(cmp_res_2);
    __m128i cmp_res_3 = _mm_cmpeq_epi8(this->v3, mask);
    uint64_t res_3 = _mm_movemask_epi8(cmp_res_3);
    return res_0 | (res_1 << 16) | (res_2 << 32) | (res_3 << 48);
  }

  really_inline uint64_t lteq(uint8_t m) {
    const __m128i maxval = _mm_set1_epi8(m);
    __m128i cmp_res_0 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, this->v0), maxval);
    uint64_t res_0 = _mm_movemask_epi8(cmp_res_0);
    __m128i cmp_res_1 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, this->v1), maxval);
    uint64_t res_1 = _mm_movemask_epi8(cmp_res_1);
    __m128i cmp_res_2 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, this->v2), maxval);
    uint64_t res_2 = _mm_movemask_epi8(cmp_res_2);
    __m128i cmp_res_3 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, this->v3), maxval);
    uint64_t res_3 = _mm_movemask_epi8(cmp_res_3);
    return res_0 | (res_1 << 16) | (res_2 << 32) | (res_3 << 48);
  }

}; // struct simd_input

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_WESTMERE_SIMD_INPUT_H
