#ifndef SIMDJSON_GENERIC_JSONCHARUTILS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_JSONCHARUTILS_H
#include "simdjson/generic/base.h"
#include "simdjson/internal/jsoncharutils_tables.h"
#include "simdjson/internal/numberparsing_tables.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace jsoncharutils {

// return non-zero if not a structural or whitespace char
// zero otherwise
simdjson_inline uint32_t is_not_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace_negated[c];
}

simdjson_inline uint32_t is_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace[c];
}

// returns a value with the high 16 bits set if not valid
// otherwise returns the conversion of the 4 hex digits at src into the bottom
// 16 bits of the 32-bit return register
//
// see
// https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
static inline uint32_t hex_to_u32_nocheck(
    const uint8_t *src) { // strictly speaking, static inline is a C-ism
  uint32_t v1 = internal::digit_to_val32[630 + src[0]];
  uint32_t v2 = internal::digit_to_val32[420 + src[1]];
  uint32_t v3 = internal::digit_to_val32[210 + src[2]];
  uint32_t v4 = internal::digit_to_val32[0 + src[3]];
  return v1 | v2 | v3 | v4;
}

// given a code point cp, writes to c
// the utf-8 code, outputting the length in
// bytes, if the length is zero, the code point
// is invalid
//
// This can possibly be made faster using pdep
// and clz and table lookups, but JSON documents
// have few escaped code points, and the following
// function looks cheap.
//
// Note: we assume that surrogates are treated separately
//
simdjson_inline size_t codepoint_to_utf8(uint32_t cp, uint8_t *c) {
  if (cp <= 0x7F) {
    c[0] = uint8_t(cp);
    return 1; // ascii
  }
  if (cp <= 0x7FF) {
    c[0] = uint8_t((cp >> 6) + 192);
    c[1] = uint8_t((cp & 63) + 128);
    return 2; // universal plane
    //  Surrogates are treated elsewhere...
    //} //else if (0xd800 <= cp && cp <= 0xdfff) {
    //  return 0; // surrogates // could put assert here
  } else if (cp <= 0xFFFF) {
    c[0] = uint8_t((cp >> 12) + 224);
    c[1] = uint8_t(((cp >> 6) & 63) + 128);
    c[2] = uint8_t((cp & 63) + 128);
    return 3;
  } else if (cp <= 0x10FFFF) { // if you know you have a valid code point, this
                               // is not needed
    c[0] = uint8_t((cp >> 18) + 240);
    c[1] = uint8_t(((cp >> 12) & 63) + 128);
    c[2] = uint8_t(((cp >> 6) & 63) + 128);
    c[3] = uint8_t((cp & 63) + 128);
    return 4;
  }
  // will return 0 when the code point was too large.
  return 0; // bad r
}

#if SIMDJSON_IS_32BITS // _umul128 for x86, arm
// this is a slow emulation routine for 32-bit
//
static simdjson_inline uint64_t __emulu(uint32_t x, uint32_t y) {
  return x * (uint64_t)y;
}
static simdjson_inline uint64_t _umul128(uint64_t ab, uint64_t cd, uint64_t *hi) {
  uint64_t ad = __emulu((uint32_t)(ab >> 32), (uint32_t)cd);
  uint64_t bd = __emulu((uint32_t)ab, (uint32_t)cd);
  uint64_t adbc = ad + __emulu((uint32_t)ab, (uint32_t)(cd >> 32));
  uint64_t adbc_carry = !!(adbc < ad);
  uint64_t lo = bd + (adbc << 32);
  *hi = __emulu((uint32_t)(ab >> 32), (uint32_t)(cd >> 32)) + (adbc >> 32) +
        (adbc_carry << 32) + !!(lo < bd);
  return lo;
}
#endif

} // namespace jsoncharutils
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_JSONCHARUTILS_H