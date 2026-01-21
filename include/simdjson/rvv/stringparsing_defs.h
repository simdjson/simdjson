#ifndef SIMDJSON_RVV_STRINGPARSING_DEFS_H
#define SIMDJSON_RVV_STRINGPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/rvv/base.h"
#include "simdjson/rvv/simd.h"
#include "simdjson/rvv/bitmanipulation.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace rvv {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 64;
  simdjson_inline backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  // True if a quote appears before the first backslash.
  simdjson_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }

  // For the 64-byte fast path, we only care about backslashes that occur before the first quote
  // (once we're inside a string, escape handling is done by the generic escape scanner).
  simdjson_inline bool has_backslash() { return ((quote_bits - 1) & bs_bits) != 0; }

  simdjson_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint64_t bs_bits;
  uint64_t quote_bits;
}; // struct backslash_and_quote

simdjson_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // May read up to (BYTES_PROCESSED - 1) bytes beyond the end; requires SIMDJSON_PADDING.
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1),
                "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");

  simd8<uint8_t> v(src);
  // store to dest unconditionally - we can overwrite the bits we don't like later
  v.store(dst);

  return {
    uint64_t((v == '\\').to_bitmask()), // bs_bits
    uint64_t((v == '\"').to_bitmask())  // quote_bits
  };
}

// Finds the characters that must be escaped within a string (quote, backslash, and control chars).
struct escaping {
  static constexpr uint32_t BYTES_PROCESSED = 64;
  simdjson_inline static escaping copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_escape() { return escape_bits != 0; }
  simdjson_inline int escape_index() { return trailing_zeroes(escape_bits); }

  uint64_t escape_bits;
}; // struct escaping

simdjson_inline escaping escaping::copy_and_find(const uint8_t *src, uint8_t *dst) {
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1),
                "escaping finder must process fewer than SIMDJSON_PADDING bytes");

  simd8<uint8_t> v(src);
  v.store(dst);

  simd8<bool> is_quote = (v == '\"');
  simd8<bool> is_backslash = (v == '\\');
  // JSON control characters are < 0x20. Use <= 0x1f to avoid needing operator<.
  simd8<bool> is_control = (v <= uint8_t(0x1f));

  return { uint64_t((is_backslash | is_quote | is_control).to_bitmask()) };
}

} // unnamed namespace
} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_STRINGPARSING_DEFS_H
