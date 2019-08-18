#ifndef SIMDJSON_HASWELL_STRINGPARSING_H
#define SIMDJSON_HASWELL_STRINGPARSING_H

#include "../stringparsing.h"

#ifdef IS_X86_64
TARGET_HASWELL
namespace simdjson {
template <>
really_inline parse_string_helper
find_bs_bits_and_quote_bits<Architecture::HASWELL>(const uint8_t *src,
                                                   uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(__m256i) - 1 <= SIMDJSON_PADDING);
  __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
  // store to dest unconditionally - we can overwrite the bits we don't like
  // later
  _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), v);
  auto quote_mask = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('"'));
  return {
      static_cast<uint32_t>(_mm256_movemask_epi8(
          _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\\')))),     // bs_bits
      static_cast<uint32_t>(_mm256_movemask_epi8(quote_mask)) // quote_bits
  };
}
} // namespace simdjson
UNTARGET_REGION

#define TARGETED_ARCHITECTURE Architecture::HASWELL
#define TARGETED_REGION TARGET_HASWELL
#include "generic/stringparsing.h"
#undef TARGETED_ARCHITECTURE
#undef TARGETED_REGION

#endif // IS_X86_64

#endif
