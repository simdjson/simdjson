#ifndef SIMDJSON_GENERIC_ATOMPARSING_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ATOMPARSING_H
#include "simdjson/generic/base.h"
#include "simdjson/generic/jsoncharutils.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <cstring>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
/// @private
namespace atomparsing {

// The string_to_uint32 is exclusively used to map literal strings to 32-bit values.
// We use memcpy instead of a pointer cast to avoid undefined behaviors since we cannot
// be certain that the character pointer will be properly aligned.
// You might think that using memcpy makes this function expensive, but you'd be wrong.
// All decent optimizing compilers (GCC, clang, Visual Studio) will compile string_to_uint32("false");
// to the compile-time constant 1936482662.
simdjson_inline uint32_t string_to_uint32(const char* str) { uint32_t val; std::memcpy(&val, str, sizeof(uint32_t)); return val; }

// Acts on the same principle as string_to_uint32, but on an 8-byte block of memory
simdjson_inline uint64_t string_to_uint64(const char* str) { uint64_t val; std::memcpy(&val, str, sizeof(uint64_t)); return val; }


// Again in str4ncmp we use a memcpy to avoid undefined behavior. The memcpy may appear expensive.
// Yet all decent optimizing compilers will compile memcpy to a single instruction, just about.
simdjson_warn_unused
simdjson_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

// Checks that the first 8 characters of the input string match the given atom in a case-insensitive manner.
//
// 'atom' must consist of only lowercase letters.
simdjson_warn_unused
simdjson_inline uint64_t str8ncmp_case_insensitive(const uint8_t *src, const char* atom) {
  uint64_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint64_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 8 bytes");
  std::memcpy(&srcval, src, sizeof(uint64_t));

  return (srcval | 0x2020202020202020ull) ^ string_to_uint64(atom);
}

// Checks that the first 3 characters of 'src' match 'atom' in a case-insensitive way.
//
// 'atom' must consist of only lowercase letters.
simdjson_warn_unused
simdjson_inline uint32_t str3ncmp_case_insensitive(const uint8_t *src, const char* atom) {
  return ((src[0] | 0x20) ^ atom[0]) //
       | ((src[1] | 0x20) ^ atom[1]) //
       | ((src[2] | 0x20) ^ atom[2]);
}

simdjson_warn_unused
simdjson_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | jsoncharutils::is_not_structural_or_whitespace(src[5])) == 0;
}

simdjson_warn_unused
simdjson_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

#if SIMDJSON_ENABLE_NAN_INF
// "nan" is 3 bytes; we check characters and then verify the next
// character is structural or whitespace. We accept both "nan" and "NaN".
simdjson_warn_unused
simdjson_inline bool is_valid_nan_atom(const uint8_t *src) {
  return (str3ncmp_case_insensitive(src, "nan")
        | jsoncharutils::is_not_structural_or_whitespace(src[3])) == 0;
}

// checks that the next four characters of a string are 'nan"', where the 'nan'
// is checked in a case-insensitive way.
simdjson_warn_unused
simdjson_inline bool is_valid_nan_in_string(const uint8_t *src) {
  return (str3ncmp_case_insensitive(src, "nan") | (src[3] ^ '"')) == 0;
}

simdjson_warn_unused
simdjson_inline bool is_valid_nan_atom(const uint8_t *src, size_t len) {
  if (len > 3) { return is_valid_nan_atom(src); }
  if (len == 3) { return str3ncmp_case_insensitive(src, "nan") == 0; }
  return false;
}

// This function will accept any case-insensitive 3-character spelling of
// infinity: 'inf', 'INF', and 'Inf' are all accepted.
//
// Any capitalization of 'infinity' is also accepted.
simdjson_warn_unused
simdjson_inline bool is_valid_inf_atom(const uint8_t *src) {
  bool is_short_inf = (str3ncmp_case_insensitive(src, "inf")
                     | jsoncharutils::is_not_structural_or_whitespace(src[3])) == 0;
  if(is_short_inf) return true;

  // Check for 'infinity' (any capitalization)
  return (str8ncmp_case_insensitive(src, "infinity") | jsoncharutils::is_not_structural_or_whitespace(src[8])) == 0;
}

simdjson_warn_unused
simdjson_inline bool is_valid_inf_in_string(const uint8_t *src) {
  bool is_short_inf = (str3ncmp_case_insensitive(src, "inf") | (src[3] ^ '"')) == 0;
  if(is_short_inf) return true;

  return (str8ncmp_case_insensitive(src, "infinity") | (src[8] ^ '"')) == 0;
}


// This function will accept any case-insensitive 3-character spelling of
// infinity: 'inf', 'INF', and 'Inf' are all accepted.
//
// Any capitalization of 'infinity' is also accepted.
simdjson_warn_unused
simdjson_inline bool is_valid_inf_atom(const uint8_t *src, size_t len) {
  if (len > 8) { return is_valid_inf_atom(src); }
  if (len == 8) { return str8ncmp_case_insensitive(src, "infinity") == 0; }
  if (len > 3) {
    return (str3ncmp_case_insensitive(src, "inf")
          | jsoncharutils::is_not_structural_or_whitespace(src[3])) == 0;
  }
  if (len == 3) { return str3ncmp_case_insensitive(src, "inf") == 0; }
  return false;
}
#endif // SIMDJSON_ENABLE_NAN_INF

} // namespace atomparsing
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_ATOMPARSING_H
