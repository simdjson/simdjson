#ifndef SIMDJSON_ARM64_STRINGPARSING_DEFS_H
#define SIMDJSON_ARM64_STRINGPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/arm64/base.h"
#include "simdjson/arm64/simd.h"
#include "simdjson/arm64/bitmanipulation.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace arm64 {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 64;
  simdjson_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_inline bool has_backslash() { return bs_bits != 0; }
  simdjson_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint64_t bs_bits;
  uint64_t quote_bits;
}; // struct backslash_and_quote

simdjson_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 63 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  simd8x64<uint8_t> v(src);
  // store to dest unconditionally - we can overwrite the bits we don't like later
  v.store(dst);
  return {
    v.eq('\\'), // bs_bits
    v.eq('"')   // quote_bits
  };
}

} // unnamed namespace
} // namespace arm64
} // namespace simdjson

#endif // SIMDJSON_ARM64_STRINGPARSING_DEFS_H
