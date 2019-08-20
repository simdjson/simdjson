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

  template <typename F>
  really_inline uint64_t build_bitmask(F const& chunk_to_mask) {
    uint64_t r0 = static_cast<uint32_t>(_mm_movemask_epi8(chunk_to_mask(this->v0)));
    uint64_t r1 =                       _mm_movemask_epi8(chunk_to_mask(this->v1));
    uint64_t r2 =                       _mm_movemask_epi8(chunk_to_mask(this->v2));
    uint64_t r3 =                       _mm_movemask_epi8(chunk_to_mask(this->v3));
    return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
  }

  really_inline uint64_t eq(uint8_t m) {
    const __m128i mask = _mm_set1_epi8(m);
    return this->build_bitmask([&](auto chunk) {
      return _mm_cmpeq_epi8(chunk, mask);
    });
  }

  really_inline uint64_t lteq(uint8_t m) {
    const __m128i maxval = _mm_set1_epi8(m);
    return this->build_bitmask([&](auto chunk) {
      return _mm_cmpeq_epi8(_mm_max_epu8(maxval, chunk), maxval);
    });
  }

}; // struct simd_input

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_WESTMERE_SIMD_INPUT_H
