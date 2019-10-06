#ifndef SIMDJSON_HASWELL_STRINGPARSING_H
#define SIMDJSON_HASWELL_STRINGPARSING_H

#ifdef IS_X86_64

#include "simdjson/common_defs.h"
#include "simdjson/jsoncharutils.h"
#include "simdjson/parsedjson.h"
#include "haswell/simdutf8check.h"

#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

TARGET_HASWELL
namespace simdjson::haswell {

// Holds backslashes and quotes locations.
struct scanned_string {
  uint32_t bs_bits;
  uint32_t quote_bits;
  really_inline uint32_t bytes_scanned() const { return sizeof(__m256i); }
};

really_inline __m256i load_with_padding(const uint8_t * src, const uint8_t *src_end) {
  // If we're at the last bit of input, we only load part of it and pad the rest with spaces, so
  // that the utf8 processor can do its magic.
  size_t remaining_src = src_end - src;
  if (unlikely(remaining_src < sizeof(__m256i))) {
    uint8_t tmp_src[sizeof(__m256i)];
    memset(tmp_src, ' ', sizeof(__m256i));
    memcpy(tmp_src, src, remaining_src);
    return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(tmp_src));
  }

  // Otherwise, just load from the normal place.
  return  _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
}

really_inline scanned_string scan_string(const uint8_t *src, uint8_t *dst, const uint8_t *src_end, utf8_checker &utf8) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(__m256i) - 1 <= SIMDJSON_PADDING);
  __m256i v = load_with_padding(src, src_end);

  // store to dest unconditionally - we can overwrite the bits we don't like later
  _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), v);
  auto quote_mask = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('"'));
  utf8.check_next_input(v);
  return {
      static_cast<uint32_t>(_mm256_movemask_epi8(
          _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\\')))),     // bs_bits
      static_cast<uint32_t>(_mm256_movemask_epi8(quote_mask)) // quote_bits
  };
}

#include "generic/stringparsing.h"

} // namespace simdjson::haswell
UNTARGET_REGION

#endif // IS_X86_64

#endif
