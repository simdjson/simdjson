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

  really_inline simd_input(__m256i a_lo, __m256i a_hi) {
    this->lo = a_lo;
    this->hi = a_hi;
  }

  template <typename F>
  really_inline simd_input<Architecture::HASWELL> map(F const& map_chunk) {
    return simd_input<Architecture::HASWELL>(
      map_chunk(this->lo),
      map_chunk(this->hi)
    );
  }

  really_inline uint64_t to_bitmask() {
    uint64_t r_lo = static_cast<uint32_t>(_mm256_movemask_epi8(this->lo));
    uint64_t r_hi =                       _mm256_movemask_epi8(this->hi);
    return r_lo | (r_hi << 32);
  }

  really_inline uint64_t eq(uint8_t m) {
    const __m256i mask = _mm256_set1_epi8(m);
    return this->MAP_BITMASK( _mm256_cmpeq_epi8(chunk, mask) );
  }

  really_inline uint64_t lteq(uint8_t m) {
    const __m256i maxval = _mm256_set1_epi8(m);
    return this->MAP_BITMASK( _mm256_cmpeq_epi8(_mm256_max_epu8(maxval, chunk), maxval) );
  }

}; // struct simd_input

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_HASWELL_SIMD_INPUT_H
