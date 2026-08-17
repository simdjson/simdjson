#ifndef SIMDJSON_FALLBACK_STRINGPARSING_DEFS_H
#define SIMDJSON_FALLBACK_STRINGPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/fallback/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace fallback {
namespace {

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 1;
  simdjson_inline backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  uint8_t c;
}; // struct backslash_and_quote

// The scalar fallback processes one byte per chunk, so its accessors inspect the
// single stored byte rather than bitmasks. These non-template overloads are
// preferred over the shared bitmask templates in src/generic/stage2/stringparsing.h
// (which require `bs_bits`/`quote_bits` members that fallback does not have).
simdjson_inline bool has_quote_first(const backslash_and_quote &b) { return b.c == '"'; }
simdjson_inline bool has_backslash(const backslash_and_quote &b) { return b.c == '\\'; }
simdjson_inline int quote_index(const backslash_and_quote &b) { return b.c == '"' ? 0 : 1; }
simdjson_inline int backslash_index(const backslash_and_quote &b) { return b.c == '\\' ? 0 : 1; }

simdjson_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // store to dest unconditionally - we can overwrite the bits we don't like later
  dst[0] = src[0];
  return { src[0] };
}


struct escaping {
  static constexpr uint32_t BYTES_PROCESSED = 1;
  simdjson_inline static escaping copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_escape() { return escape_bits; }
  simdjson_inline int escape_index() { return 0; }

  bool escape_bits;
}; // struct escaping



simdjson_inline escaping escaping::copy_and_find(const uint8_t *src, uint8_t *dst) {
  dst[0] = src[0];
  return { (src[0] == '\\') || (src[0] == '"') || (src[0] < 32) };
}

} // unnamed namespace
} // namespace fallback
} // namespace simdjson

#endif // SIMDJSON_FALLBACK_STRINGPARSING_DEFS_H
