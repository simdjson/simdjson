#ifndef SIMDJSON_RVV_VLS_STRINGPARSING_DEFS_H
#define SIMDJSON_RVV_VLS_STRINGPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/rvv-vls/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace rvv_vls {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint64_t BYTES_PROCESSED = sizeof(simd8<uint8_t>);
  simdjson_inline backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_inline bool has_backslash() { return ((quote_bits - 1) & bs_bits) != 0; }
  simdjson_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint64_t bs_bits;
  uint64_t quote_bits;
}; // struct backslash_and_quote

simdjson_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v(src);
  v.store(dst);
  return { (v == '\\').to_bitmask(), (v == '"').to_bitmask() };
}

struct escaping {
  static constexpr uint64_t BYTES_PROCESSED = sizeof(simd8<uint8_t>);
  simdjson_inline static escaping copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_escape() { return escape_bits != 0; }
  simdjson_inline int escape_index() { return trailing_zeroes(escape_bits) / 4; }

  uint64_t escape_bits;
}; // struct escaping

simdjson_inline escaping escaping::copy_and_find(const uint8_t *src, uint8_t *dst) {
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "escaping finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v(src);
  v.store(dst);
  return { ((v == '"') | (v == '\\') | (v == 32)).to_bitmask() };
}


} // unnamed namespace
} // namespace rvv_vls
} // namespace simdjson

#endif // SIMDJSON_RVV_VLS_STRINGPARSING_DEFS_H
