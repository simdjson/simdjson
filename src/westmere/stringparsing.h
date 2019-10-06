#ifndef SIMDJSON_WESTMERE_STRINGPARSING_H
#define SIMDJSON_WESTMERE_STRINGPARSING_H

#ifdef IS_X86_64

#include "simdjson/common_defs.h"
#include "simdjson/jsoncharutils.h"
#include "simdjson/parsedjson.h"
#include "westmere/simdutf8check.h"

#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

TARGET_WESTMERE
namespace simdjson::westmere {

// Holds backslashes and quotes locations.
struct scanned_string {
  uint32_t bs_bits;
  uint32_t quote_bits;
  really_inline uint32_t bytes_scanned() const { return sizeof(__m128i); }
};

really_inline __m128i load_with_padding(const uint8_t * src, const uint8_t *src_end) {
  // If we're at the last bit of input, we only load part of it and pad the rest with spaces, so
  // that the utf8 processor can do its magic.
  size_t remaining_src = src_end - src;
  if (unlikely(remaining_src < sizeof(__m128i))) {
    uint8_t tmp_src[sizeof(__m128i)];
    memset(tmp_src, ' ', sizeof(__m128i));
    memcpy(tmp_src, src, remaining_src);
    return _mm_loadu_si128(reinterpret_cast<const __m128i *>(tmp_src));
  }

  // Otherwise, just load from the normal place.
  return  _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
}

really_inline scanned_string scan_string(const uint8_t *src, uint8_t *dst, const uint8_t *src_end, utf8_checker &utf8) {
  __m128i v = load_with_padding(src, src_end);
  // store to dest unconditionally - we can overwrite the bits we don't like
  // later
  _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), v);
  auto quote_mask = _mm_cmpeq_epi8(v, _mm_set1_epi8('"'));
  utf8.check_next_input(v);
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
