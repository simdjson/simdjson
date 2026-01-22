#ifndef SIMDJSON_ICELAKE_STRINGPARSING_DEFS_H
#define SIMDJSON_ICELAKE_STRINGPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/icelake/base.h"
#include "simdjson/icelake/simd.h"
#include "simdjson/icelake/bitmanipulation.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace icelake {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 64;
  simdjson_inline backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_inline bool has_backslash() { return ((quote_bits - 1) & bs_bits) != 0; }
  simdjson_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint64_t bs_bits;
  uint64_t quote_bits;
}; // struct backslash_and_quote

simdjson_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 15 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v(src);
  // store to dest unconditionally - we can overwrite the bits we don't like later
  v.store(dst);
  return {
      static_cast<uint64_t>(v == '\\'), // bs_bits
      static_cast<uint64_t>(v == '"'), // quote_bits
  };
}



struct escaping {
  static constexpr uint32_t BYTES_PROCESSED = 64;
  simdjson_inline static escaping copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_escape() { return escape_bits != 0; }
  simdjson_inline int escape_index() { return trailing_zeroes(uint64_t(escape_bits)); }

  __mmask64 escape_bits;
}; // struct escaping



simdjson_inline escaping escaping::copy_and_find(const uint8_t *src, uint8_t *dst) {
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "escaping finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v(src);
  v.store(dst);
  __mmask64 is_quote = _mm512_cmpeq_epi8_mask(v, _mm512_set1_epi8('"'));
  __mmask64 is_backslash = _mm512_cmpeq_epi8_mask(v, _mm512_set1_epi8('\\'));
  __mmask64 is_control = _mm512_cmplt_epi8_mask(v, _mm512_set1_epi8(32));
  return {
    (is_backslash | is_quote | is_control)
  };
}




} // unnamed namespace
} // namespace icelake
} // namespace simdjson

#endif // SIMDJSON_ICELAKE_STRINGPARSING_DEFS_H
