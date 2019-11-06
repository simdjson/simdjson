#ifndef SIMDJSON_HASWELL_STRINGPARSING_H
#define SIMDJSON_HASWELL_STRINGPARSING_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "haswell/simd.h"
#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"
#include "jsoncharutils.h"

TARGET_HASWELL
namespace simdjson::haswell {

using namespace simd;

// Holds backslashes and quotes locations.
struct parse_string_helper {
  uint32_t bs_bits;
  uint32_t quote_bits;
  static const uint32_t BYTES_PROCESSED = 32;
};

really_inline parse_string_helper find_bs_bits_and_quote_bits(const uint8_t *src, uint8_t *dst) {
  // this can read up to 15 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (parse_string_helper::BYTES_PROCESSED - 1));
  simd8<uint8_t> v(src);
  // store to dest unconditionally - we can overwrite the bits we don't like later
  v.store(dst);
  return {
      (uint32_t)(v == '\\').to_bitmask(),     // bs_bits
      (uint32_t)(v == '"').to_bitmask(), // quote_bits
  };
}

#include "generic/stringparsing.h"

} // namespace simdjson::haswell
UNTARGET_REGION

#endif // IS_X86_64

#endif
