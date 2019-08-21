#ifndef SIMDJSON_WESTMERE_STRINGPARSING_H
#define SIMDJSON_WESTMERE_STRINGPARSING_H

#include "../stringparsing.h"

#ifdef IS_X86_64

#include "westmere/architecture.h"

TARGET_WESTMERE
namespace simdjson::westmere {

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
