#ifndef SIMDJSON_WESTMERE_STRINGPARSING_DEFS_H
#define SIMDJSON_WESTMERE_STRINGPARSING_DEFS_H

#include "simdjson/westmere/bitmanipulation.h"
#include "simdjson/westmere/simd.h"

namespace simdjson {
namespace westmere {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_inline backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_inline bool has_backslash() { return bs_bits != 0; }
  simdjson_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint32_t bs_bits;
  uint32_t quote_bits;
}; // struct backslash_and_quote

simdjson_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v0(src);
  simd8<uint8_t> v1(src + 16);
  v0.store(dst);
  v1.store(dst + 16);
  uint64_t bs_and_quote = simd8x64<bool>(v0 == '\\', v1 == '\\', v0 == '"', v1 == '"').to_bitmask();
  return {
    uint32_t(bs_and_quote),      // bs_bits
    uint32_t(bs_and_quote >> 32) // quote_bits
  };
}


struct escaping {
  static constexpr uint32_t BYTES_PROCESSED = 16;
  simdjson_inline static escaping copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_escape() { return escape_bits != 0; }
  simdjson_inline int escape_index() { return trailing_zeroes(escape_bits); }

  uint64_t escape_bits;
}; // struct escaping



simdjson_inline escaping escaping::copy_and_find(const uint8_t *src, uint8_t *dst) {
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "escaping finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v(src);
  v.store(dst);
  simd8<bool> is_quote = (v == '"');
  simd8<bool> is_backslash = (v == '\\');
  simd8<bool> is_control = (v < 32);
  return {
    uint64_t((is_backslash | is_quote | is_control).to_bitmask())
  };
}

} // unnamed namespace
} // namespace westmere
} // namespace simdjson

#endif // SIMDJSON_WESTMERE_STRINGPARSING_DEFS_H
