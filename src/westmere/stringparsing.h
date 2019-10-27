#ifndef SIMDJSON_WESTMERE_STRINGPARSING_H
#define SIMDJSON_WESTMERE_STRINGPARSING_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"
#include "jsoncharutils.h"

#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

TARGET_WESTMERE
namespace simdjson::westmere {

// Holds backslashes and quotes locations.
struct parse_string_helper {
  uint32_t bs_bits;
  uint32_t quote_bits;
  really_inline uint32_t bytes_processed() const { return sizeof(__m128i); }
};

really_inline parse_string_helper find_bs_bits_and_quote_bits(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
  // store to dest unconditionally - we can overwrite the bits we don't like
  // later
  _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), v);
  auto quote_mask = _mm_cmpeq_epi8(v, _mm_set1_epi8('"'));
  return {
      static_cast<uint32_t>(
          _mm_movemask_epi8(_mm_cmpeq_epi8(v, _mm_set1_epi8('\\')))), // bs_bits
      static_cast<uint32_t>(_mm_movemask_epi8(quote_mask)) // quote_bits
  };
}

#include "generic/stringparsing.h"

} // namespace simdjson::westmere
UNTARGET_REGION

#endif // IS_X86_64

#endif
