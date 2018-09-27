#pragma once


#include "jsonparser/common_defs.h"
#include "jsonparser/simdjson_internal.h"

// structural chars here are
// they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
// we are also interested in the four whitespace characters
// space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d

// these are the chars that can follow a true/false/null or number atom
// and nothing else
const u32 structural_or_whitespace_negated[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// return non-zero if not a structural or whitespace char
// zero otherwise
really_inline u32 is_not_structural_or_whitespace(u8 c) {
  return structural_or_whitespace_negated[c];
}

inline bool is_hex_digit(u8 v) {
  if (v >= '0' && v <= '9')
    return true;
  v &= 0xdf;
  if (v >= 'A' && v <= 'F')
    return true;
  return false;
}

inline u8 digit_to_val(u8 v) {
  if (v >= '0' && v <= '9')
    return v - '0';
  v &= 0xdf;
  return v - 'A' + 10;
}

inline bool hex_to_u32(const u8 *src, u32 *res) {
  u8 v1 = src[0];
  u8 v2 = src[1];
  u8 v3 = src[2];
  u8 v4 = src[3];
  if (!is_hex_digit(v1) || !is_hex_digit(v2) || !is_hex_digit(v3) ||
      !is_hex_digit(v4)) {
    return false;
  }
  *res = digit_to_val(v1) << 24 | digit_to_val(v2) << 16 |
         digit_to_val(v3) << 8 | digit_to_val(v4);
  return true;
}