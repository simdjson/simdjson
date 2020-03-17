#ifndef SIMDJSON_FALLBACK_STRINGPARSING_H
#define SIMDJSON_FALLBACK_STRINGPARSING_H

#include "simdjson.h"
#include "jsoncharutils.h"

namespace simdjson::fallback {

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 1;
  really_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  really_inline bool has_quote_first() { return c == '"'; }
  really_inline bool has_backslash() { return c == '\\'; }
  really_inline int quote_index() { return c == '"' ? 0 : 1; }
  really_inline int backslash_index() { return c == '\\' ? 0 : 1; }

  uint8_t c;
}; // struct backslash_and_quote

really_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // store to dest unconditionally - we can overwrite the bits we don't like later
  dst[0] = src[0];
  return { src[0] };
}

#include "generic/stringparsing.h"

} // namespace simdjson::fallback

#endif // SIMDJSON_FALLBACK_STRINGPARSING_H
