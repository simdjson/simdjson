#ifndef SIMDJSON_PPC64_STRINGPARSING_DEFS_H
#define SIMDJSON_PPC64_STRINGPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/ppc64/base.h"
#include "simdjson/ppc64/bitmanipulation.h"
#include "simdjson/ppc64/simd.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace ppc64 {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_inline static backslash_and_quote
  copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_quote_first() {
    return ((bs_bits - 1) & quote_bits) != 0;
  }
  simdjson_inline bool has_backslash() { return bs_bits != 0; }
  simdjson_inline int quote_index() {
    return trailing_zeroes(quote_bits);
  }
  simdjson_inline int backslash_index() {
    return trailing_zeroes(bs_bits);
  }

  uint32_t bs_bits;
  uint32_t quote_bits;
}; // struct backslash_and_quote

simdjson_inline backslash_and_quote
backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1),
                "backslash and quote finder must process fewer than "
                "SIMDJSON_PADDING bytes");
  simd8<uint8_t> v0(src);
  simd8<uint8_t> v1(src + sizeof(v0));
  v0.store(dst);
  v1.store(dst + sizeof(v0));

  // Getting a 64-bit bitmask is much cheaper than multiple 16-bit bitmasks on
  // PPC; therefore, we smash them together into a 64-byte mask and get the
  // bitmask from there.
  uint64_t bs_and_quote =
      simd8x64<bool>(v0 == '\\', v1 == '\\', v0 == '"', v1 == '"').to_bitmask();
  return {
      uint32_t(bs_and_quote),      // bs_bits
      uint32_t(bs_and_quote >> 32) // quote_bits
  };
}

} // unnamed namespace
} // namespace ppc64
} // namespace simdjson

#endif // SIMDJSON_PPC64_STRINGPARSING_DEFS_H
