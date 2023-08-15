#ifndef SIMDJSON_SRC_GENERIC_STAGE1_JSON_SCANNER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_JSON_SCANNER_H
#include <generic/stage1/base.h>
#include <generic/json_character_block.h>
#include <generic/stage1/json_string_scanner.h>
#include <simdjson/generic/lookup_table.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

/**
 * Scans JSON for important bits: structural characters or 'operators', strings, and scalars.
 *
 * The scanner starts by calculating two distinct things:
 * - string characters (taking \" into account)
 * - structural characters or 'operators' ([]{},:, comma)
 *   and scalars (runs of non-operators like 123, true and "abc")
 *
 * To minimize data dependency (a key component of the scanner's speed), it finds these in parallel:
 * in particular, the operator/scalar bit will find plenty of things that are actually part of
 * strings. When we're done, json_block will fuse the two together by masking out tokens that are
 * part of a string.
 */
class json_scanner {
public:
  json_scanner() = default;
  simdjson_inline uint64_t next(const simd::simd8x64<uint8_t>& in) noexcept;
  // Returns either UNCLOSED_STRING or SUCCESS
  simdjson_inline error_code finish() const noexcept;

private:
  enum ws_op {
    COMMA     = 1 << 0,
    COLON     = 1 << 1,
    OPEN      = 1 << 2,
    CLOSE     = 1 << 3,
    QUOTE     = 1 << 4,
    BACKSLASH = 1 << 5,
    SPACE     = 1 << 6,
    TAB_CR_LF = 1 << 7,
    OP = COMMA | COLON | OPEN | CLOSE,
    SEP = COMMA | COLON,
    WS = SPACE | TAB_CR_LF,
  };
  static simdjson_constinit byte_classifier classifier = {
    { ',',  COMMA },
    { ':',  COLON },
    { '[',  OPEN },
    { '{',  OPEN },
    { ']',  CLOSE },
    { '}',  CLOSE },
    { '\"', QUOTE },
    { ' ',  SPACE },
    { '\t', TAB_CR_LF },
    { '\r', TAB_CR_LF },
    { '\n', TAB_CR_LF },
    { '\\', BACKSLASH },
  };

  simdjson_inline uint64_t next_unescaped_quotes(const simd::simd8x64<uint8_t>& in) noexcept;
  simdjson_inline uint64_t next_separated_values(uint64_t sep_open, uint64_t scalar_close) noexcept;
  simdjson_inline uint64_t next_in_string(uint64_t quote, uint64_t separated_values) noexcept;
  simdjson_inline void check_errors(uint64_t sep_open, uint64_t scalar_close, uint64_t open_close, uint64_t quote, uint64_t separated_values, uint64_t in_string) noexcept;

  // Whether the last character of the previous iteration is part of a scalar token
  // (anything except whitespace or a structural character/'operator').
  json_escape_scanner escape_scanner{};
  bitmask_stream::subtract split_values_by_separator{};
  bitmask_stream::subtract::overflow_t still_in_string{};
  bitmask_stream::shift_forward<1> shift_in_scalar{}; // for error calculation
  uint64_t error{};
};

simdjson_inline uint64_t json_scanner::next(const simd::simd8x64<uint8_t>& in) noexcept {
  uint64_t quote = next_unescaped_quotes(in);
  // shortest path =     6 (6N+5) or 12 (6N+12)

  // Classify bytes into comma, open, close, sep, and whitespace.
  simd8x64<uint8_t> classified = classifier.classify(in);                           // 2N+1
  uint64_t sep_open         = classified.any_bits_set(SEP | OPEN).to_bitmask();     //      3N+1
  uint64_t scalar_close     = classified.no_bits_set(SEP | OPEN | WS).to_bitmask(); //      3N+1
  uint64_t open_close       = classified.any_bits_set(OPEN | CLOSE).to_bitmask();   //      3N+1
  // shortest path =     7 (11N+4 total)

  uint64_t separated_values = next_separated_values(sep_open, scalar_close);
  // shortest path = [7] 1 (2 total)

  uint64_t in_string = next_in_string(quote, separated_values);
  // shortest path = [8] 3 or 12 (7 or 20 total)

  uint64_t comma = in.eq(',');                           // 3N+1
  uint64_t op = sep_open | open_close;                   // [7] 1
  uint64_t op_without_comma = op & ~comma;               //     (ternary)
  uint64_t lead_value = scalar_close & separated_values; // [8]   1
  uint64_t all_structurals = op_without_comma | lead_value; //    (ternary)
  uint64_t structurals = all_structurals & ~in_string;   // [11]    1
  // shortest path = [11] 1 (3N+4 total)

  check_errors(sep_open, scalar_close, open_close, quote, separated_values, in_string);
  // shortest path = [11] 2 (8 total)

  return structurals;
  // structurals: shortest path = 12 (+1) (20N+30)
}

simdjson_inline uint64_t json_scanner::next_unescaped_quotes(const simd::simd8x64<uint8_t>& in) noexcept {
  // Figure out which quotes are real (unescaped)
  uint64_t backslash = in.eq('\\');                          // 3N+1
  uint64_t raw_quote = in.eq('"');                           // 3N+1
  uint64_t escaped = escape_scanner.next(backslash).escaped; //      1 (+1) or 7 (+2)
  return raw_quote & ~escaped;                               //        1
  // shortest path = 6 (6N+5 total) or 12 (6N+12 total)
}

simdjson_inline void json_scanner::check_errors(uint64_t sep_open, uint64_t scalar_close, uint64_t open_close, uint64_t quote, uint64_t separated_values, uint64_t in_string) noexcept {
  // Detect separator errors
  // ERROR: missing separator between scalars or close brackets (scalar preceded by anything other than separator, open, or beginning of document)
  uint64_t scalar = scalar_close & open_close;                         // [7] 1
  uint64_t next_in_scalar = scalar & ~quote;                           // [6]  (ternary &)
  uint64_t in_scalar = shift_in_scalar.next(next_in_scalar);           //       1 (+1)
  uint64_t first_scalar = scalar & ~in_scalar;                         //         1
  // Take away lead scalar characters, which are allowed to be the first scalar character
  uint64_t missing_separator_error = first_scalar & ~separated_values; //     (ternary)
  // shortest path = [7] 3 (4 total)

  // ERROR: separator with another separator or open bracket ahead of it (or at beginning of document)
  uint64_t sep = sep_open & ~open_close;                    // [7] 1
  uint64_t extra_separator_error = sep & ~separated_values; // [8] (ternary &)
  // ERROR: open bracket without separator ahead of it (except at beginning of document)
  uint64_t open = sep_open & open_close;                                  // [7] 1
  uint64_t missing_separator_before_open_error = open & separated_values; // [8] (ternary &)
  // Total: [8] 1 (2 total)

  //                                                                    // [8]  1 (ternary)
  uint64_t raw_separator_error = missing_separator_error | extra_separator_error | missing_separator_before_open_error;
  // flip lead quote off and trail quote on: lead quote errors
  uint64_t separator_error = raw_separator_error & (in_string ^ quote); // [11]   1 (ternary)
  this->error |= separator_error;                                       //          1
  // Total: [11] 2 (2 total)

  // NOT validated:
  // - Object/array: Brace balance / type
  // - Object: key type = string
  // - Object: Colon only between key and value
  // - Empty object/array: close bracket before separator preceded by open bracket
  // - UTF-8 in strings
  // - scalar format

  // Total: [11] 2 (8 total)
}

simdjson_inline uint64_t json_scanner::next_separated_values(uint64_t sep_open, uint64_t scalar_close) noexcept {
  // Split the JSON by separators. After this, we know:
  // - the lead character of every valid scalar.
  // - there is least one scalar/close bracket between each separator
  // - open bracket is always after separator or at beginning of the document
  // OPEN|WS* CLOSE|SCALAR (CLOSE|SCALAR|WS)* SEP OPEN|WS*
  //    1|0 *      1                   0|1  *  1     1|0 *
  // (We include open brackets with separators because we can easily detect some errors from that.)
  return split_values_by_separator.next(sep_open, scalar_close); // [7] 1 (+1)
}

simdjson_inline uint64_t json_scanner::next_in_string(
  uint64_t quote,           // [6]
  uint64_t separated_values // [8]
) noexcept {
  // Find values that are in the string. ASSUME that strings do not have separators/openers just
  // before the end of the string (i.e. "blah," or "blah,["). These are pretty rare.
  // TODO: we can also assume the carry in is 1 if the first quote is a trailing quote.
  uint64_t lead_quote     = quote &  separated_values;                 // [8] 1
  uint64_t trailing_quote = quote & ~separated_values;                 // [8] 1
  // If we were correct, the subtraction will leave us with:
  // LEAD-QUOTE=1 NON-QUOTE=1* TRAIL-QUOTE=0 NON-QUOTE=0* ...
  // The general form is this:
  // LEAD-QUOTE=1 NON-QUOTE=1|LEAD-QUOTE=0* TRAIL-QUOTE=0 NON-QUOTE=0|TRAIL-QUOTE=1* ...
  //                                                                            1 (+1)
  auto was_still_in_string = this->still_in_string;
  uint64_t in_string = bitmask_stream::subtract::next(trailing_quote, lead_quote, this->still_in_string);
  // Assumption check! LEAD-QUOTE=0 means a lead quote was inside a string--meaning the second
  // quote was preceded by a separator/open.
  uint64_t lead_quote_in_string = lead_quote & ~in_string;             //         1
  if (!lead_quote_in_string) {
    // This shouldn't happen often, so we take the heavy branch penalty for it and use the
    // high-latency prefix_xor.
    //                                                                 // [6] 12 (+1)
    this->still_in_string = was_still_in_string;
    in_string = bitmask_stream::alternating_regions::next(quote, still_in_string);
  }
  return in_string;
  // shortest path = [8] 2 or [6] 12 (5 or 18 total)
}


simdjson_inline error_code json_scanner::finish() const noexcept {
  return this->error | this->still_in_string;
}

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_JSON_SCANNER_H