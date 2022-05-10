#ifndef SIMDJSON_ICELAKE_STRINGPARSING_H
#define SIMDJSON_ICELAKE_STRINGPARSING_H

#include "simdjson/base.h"
#include "simdjson/icelake/simd.h"
#include "simdjson/icelake/bitmanipulation.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_really_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_really_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_really_inline bool has_backslash() { return ((quote_bits - 1) & bs_bits) != 0; }
  simdjson_really_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_really_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint64_t bs_bits;
  uint64_t quote_bits;
}; // struct backslash_and_quote

simdjson_really_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
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

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#include "simdjson/generic/stringparsing.h"

#endif // SIMDJSON_ICELAKE_STRINGPARSING_H
