#ifndef SIMDJSON_STRINGPARSING_H
#define SIMDJSON_STRINGPARSING_H

#include "simdjson/common_defs.h"
#include "simdjson/jsoncharutils.h"
#include "simdjson/parsedjson.h"

#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

namespace simdjson {
// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const uint8_t escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x0c, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
    0, 0, 0x0d, 0, 0x09, 0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x7.

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
};

// handle a unicode codepoint
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
WARN_UNUSED
really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr) {
  // hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the
  // multilingual plane check
  uint32_t code_point = hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = hex_to_u32_nocheck(*src_ptr + 2);

    // if the first code point is invalid we will get here, as we will go past
    // the check for being outside the Basic Multilingual plane. If we don't
    // find a \u immediately afterwards we fail out anyhow, but if we do,
    // this check catches both the case of the first code point being invalid
    // or the second code point being invalid.
    if ((code_point | code_point_2) >> 16) {
      return false;
    }

    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    *src_ptr += 6;
  }
  size_t offset = codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}

// Holds backslashes and quotes locations.
struct parse_string_helper {
  uint32_t bs_bits;
  uint32_t quote_bits;
};

// Finds where the backslashes and quotes are located.
template <Architecture>
parse_string_helper find_bs_bits_and_quote_bits(const uint8_t *src,
                                                uint8_t *dst);

template <Architecture T>
WARN_UNUSED ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER LENIENT_MEM_SANITIZER
    really_inline bool
    parse_string(UNUSED const uint8_t *buf, UNUSED size_t len, ParsedJson &pj,
                 UNUSED const uint32_t depth, UNUSED uint32_t offset);

} // namespace simdjson

/// Now include the specializations:
#include "simdjson/stringparsing_arm64.h"
#include "simdjson/stringparsing_haswell.h"
#include "simdjson/stringparsing_westmere.h"

#endif
