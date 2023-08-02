#ifndef SIMDJSON_SRC_GENERIC_STAGE1_JSON_STRING_SCANNER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_JSON_STRING_SCANNER_H
#include <generic/stage1/base.h>
#include <generic/stage1/json_escape_scanner.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

struct json_string_block {
  // We spell out the constructors in the hope of resolving inlining issues with Visual Studio 2017
  simdjson_really_inline json_string_block(uint64_t escaped, uint64_t quote, uint64_t in_string) :
  _escaped(escaped), _quote(quote), _in_string(in_string) {}

  // Escaped characters (characters following an escape() character)
  simdjson_really_inline uint64_t escaped() const { return _escaped; }
  // Real (non-backslashed) quotes
  simdjson_really_inline uint64_t quote() const { return _quote; }
  // Only characters inside the string (not including the quotes)
  simdjson_really_inline uint64_t string_content() const { return _in_string & ~_quote; }
  // Return a mask of whether the given characters are inside a string (only works on non-quotes)
  simdjson_really_inline uint64_t non_quote_inside_string(uint64_t mask) const { return mask & _in_string; }
  // Return a mask of whether the given characters are inside a string (only works on non-quotes)
  simdjson_really_inline uint64_t non_quote_outside_string(uint64_t mask) const { return mask & ~_in_string; }
  // Tail of string (everything except the start quote)
  simdjson_really_inline uint64_t string_tail() const { return _in_string ^ _quote; }

  // escaped characters (backslashed--does not include the hex characters after \u)
  uint64_t _escaped;
  // real quotes (non-escaped ones)
  uint64_t _quote;
  // string characters (includes start quote but not end quote)
  uint64_t _in_string;
};

// Scans blocks for string characters, storing the state necessary to do so
class json_string_scanner {
public:
  simdjson_really_inline json_string_block next(const simd::simd8x64<uint8_t>& in);
  // Returns either UNCLOSED_STRING or SUCCESS
  simdjson_really_inline error_code finish();

private:
  // Scans for escape characters
  json_escape_scanner escape_scanner{};
  // Whether the last iteration was still inside a string (all 1's = true, all 0's = false).
  uint64_t prev_in_string = 0ULL;
};

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
simdjson_really_inline json_string_block json_string_scanner::next(const simd::simd8x64<uint8_t>& in) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = escape_scanner.next(backslash).escaped;
  const uint64_t quote = in.eq('"') & ~escaped;

  //
  // prefix_xor flips on bits inside the string (and flips off the end quote).
  //
  // Then we xor with prev_in_string: if we were in a string already, its effect is flipped
  // (characters inside strings are outside, and characters outside strings are inside).
  //
  const uint64_t in_string = prefix_xor(quote) ^ prev_in_string;

  //
  // Check if we're still in a string at the end of the box so the next block will know
  //
  prev_in_string = uint64_t(static_cast<int64_t>(in_string) >> 63);

  // Use ^ to turn the beginning quote off, and the end quote on.

  // We are returning a function-local object so either we get a move constructor
  // or we get copy elision.
  return json_string_block(escaped, quote, in_string);
}

simdjson_really_inline error_code json_string_scanner::finish() {
  if (prev_in_string) {
    return UNCLOSED_STRING;
  }
  return SUCCESS;
}

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_JSON_STRING_SCANNER_H