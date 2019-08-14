#ifndef SIMDJSON_SIMD_INPUT_HASWELL_H
#define SIMDJSON_SIMD_INPUT_HASWELL_H

#include "simdjson/simd_input.h"

#ifdef IS_X86_64

TARGET_HASWELL
namespace simdjson {

template <>
struct simd_input<Architecture::HASWELL> {
  __m256i lo;
  __m256i hi;
};

template <>
really_inline simd_input<Architecture::HASWELL>
fill_input<Architecture::HASWELL>(const uint8_t *ptr) {
  struct simd_input<Architecture::HASWELL> in;
  in.lo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 0));
  in.hi = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 32));
  return in;
}

template <>
really_inline uint64_t cmp_mask_against_input<Architecture::HASWELL>(
    simd_input<Architecture::HASWELL> in, uint8_t m) {
  const __m256i mask = _mm256_set1_epi8(m);
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(in.lo, mask);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(in.hi, mask);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

template <>
really_inline uint64_t unsigned_lteq_against_input<Architecture::HASWELL>(
    simd_input<Architecture::HASWELL> in, uint8_t m) {
  const __m256i maxval = _mm256_set1_epi8(m);
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(_mm256_max_epu8(maxval, in.lo), maxval);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(_mm256_max_epu8(maxval, in.hi), maxval);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_SIMD_INPUT_HASWELL_H
