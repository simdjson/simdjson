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

// Scans blocks for string characters, storing the state necessary to do so
class json_string_scanner {
public:
  simdjson_inline uint64_t next(uint64_t backslash, uint64_t raw_quote, uint64_t separated_values) noexcept;
  simdjson_inline uint64_t next_unescaped_quotes(uint64_t backslash, uint64_t raw_quote) noexcept;
  simdjson_inline uint64_t next_in_string(uint64_t in_string, uint64_t separated_values) noexcept;
  // Returns either UNCLOSED_STRING or SUCCESS
  simdjson_inline error_code finish() const noexcept;

private:

  // Scans for escape characters
  json_escape_scanner escape_scanner{};
  // Whether the last iteration was still inside a string (all 1's = true, all 0's = false).
  bool still_in_string{};
  unsigned penalty_box = 0;
};

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
simdjson_inline uint64_t json_string_scanner::next(
  uint64_t backslash,                // 3+LN
  uint64_t raw_quote,                // 3+LN
  uint64_t separated_values          // 8+LN
) noexcept {
  uint64_t quote = next_unescaped_quotes(backslash, raw_quote); // 4+LN (+3) or 8+LN (+9)
  return next_in_string(quote, separated_values); // 10+LN (+6) or (14+LN or 18+LN (+8+simd:3)).
  // critical path = 10+LN (+9) or (14+LN or 18+LN (+17+simd:3))
}

simdjson_inline uint64_t json_string_scanner::next_unescaped_quotes(
  uint64_t backslash, // 3+LN
  uint64_t raw_quote  // 3+LN
) noexcept {
  uint64_t escaped = escape_scanner.next(backslash).escaped; // 3+LN (+2) or 7+LN (+8)
  return raw_quote & ~escaped;                               // 4+LN or 8+LN (+1)
  // critical path: 4+LN (+3) or 8+LN (+9)
}

simdjson_inline uint64_t json_string_scanner::next_in_string(
  uint64_t quote,           // 4+LN or 8+LN
  uint64_t separated_values // 8+LN
) noexcept {
  // Find values that are in the string. ASSUME that strings do not have separators/openers just
  // before the end of the string (i.e. "blah," or "blah,["). These are pretty rare.
  // TODO: we can also assume the carry in is 1 if the first quote is a trailing quote.
  uint64_t lead_quote     = quote &  separated_values;                 // 9+LN (+1)
  uint64_t trailing_quote = quote & ~separated_values;                 // 9+LN (+1)
  // If we were correct, the subtraction will leave us with:
  // LEAD-QUOTE=1 NON-QUOTE=1* TRAIL-QUOTE=0 NON-QUOTE=0* ...
  // The general form is this:
  // LEAD-QUOTE=1 NON-QUOTE=1|LEAD-QUOTE=0* TRAIL-QUOTE=0 NON-QUOTE=0|TRAIL-QUOTE=1* ...
  //                                                                   // 10+LN (+2)
  auto was_still_in_string = this->still_in_string;
  uint64_t in_string = bitmask::subtract_borrow(trailing_quote, lead_quote, this->still_in_string);
  // Assumption check! LEAD-QUOTE=0 means a lead quote was inside a string--meaning the second
  // quote was preceded by a separator/open.
  if (simdjson_unlikely(lead_quote & ~in_string)) {
    // This shouldn't happen often, so we take the heavy branch penalty for it and use the
    // high-latency prefix_xor.
    in_string = bitmask::prefix_xor(quote ^ was_still_in_string); // 14+LN (+1+simd:3)
    this->still_in_string = in_string >> 63;                        // 15+LN (+1)
  }
  return in_string ^ quote;                                         // flip start and end quotes
  // critical path = 10+LN (+6) or (14+LN or 18+LN (+8+simd:3)).
  // would be 14+LN or 18+LN (+2+simd:3) by itself
}


simdjson_inline error_code json_string_scanner::finish() const noexcept {
  if (still_in_string) {
    return UNCLOSED_STRING;
  }
  return SUCCESS;
}

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_JSON_STRING_SCANNER_H