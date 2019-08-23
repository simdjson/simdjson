#ifndef SIMDJSON_HASWELL_SIMD_INPUT_H
#define SIMDJSON_HASWELL_SIMD_INPUT_H

#include "../simd_input.h"

#ifdef IS_X86_64

TARGET_HASWELL
namespace simdjson {

template <>
struct simd_input<Architecture::HASWELL> {
  __m256i lo;
  __m256i hi;

  really_inline simd_input(const uint8_t *ptr) {
    this->lo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 0));
    this->hi = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 32));
  }

  really_inline simd_input(__m256i i0, __m256i i1) {
    this->lo = i0;
    this->hi = i1;
  }

  template <typename F>
  really_inline simd_input<Architecture::HASWELL> map(F const& map_chunk) {
    return simd_input<Architecture::HASWELL>(
      map_chunk(this->lo),
      map_chunk(this->hi)
    );
  }

  really_inline uint64_t to_bitmask() {
    uint64_t r0 = static_cast<uint32_t>(_mm256_movemask_epi8(this->lo));
    uint64_t r1 =                       _mm256_movemask_epi8(this->hi);
    return r0 | (r1 << 32);
  }

  really_inline uint64_t eq(uint8_t m) {
    const __m256i mask = _mm256_set1_epi8(m);
    return this->map([&] (auto chunk) {
      return _mm256_cmpeq_epi8(chunk, mask);
    }).to_bitmask();
  }

  really_inline uint64_t lteq(uint8_t m) {
    const __m256i maxval = _mm256_set1_epi8(m);
    return this->map([&] (auto chunk) {
      return _mm256_cmpeq_epi8(_mm256_max_epu8(maxval, chunk), maxval);
    }).to_bitmask();
  }

}; // struct simd_input

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_HASWELL_SIMD_INPUT_H
