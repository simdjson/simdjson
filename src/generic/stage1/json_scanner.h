#include "simdjson/icelake/bitmask.h"
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
  static simdjson_constinit byte_classifier CLASSIFIER = {
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
  static simdjson_constinit low_nibble_lookup WHITESPACE_MATCH = {
    { ' '-1, 0xFF },
    { '\t'-1, 0xFF },
    { '\r'-1, 0xFF },
    { '\n'-1, 0xFF },
  };

  simdjson_inline uint64_t next_separated_values(uint64_t sep_open, uint64_t scalar_close) noexcept;
  simdjson_inline void check_errors(uint64_t scalar, uint64_t sep, uint64_t open, uint64_t raw_quote, uint64_t separated_values, uint64_t in_string) noexcept;

  // Whether the last character of the previous iteration is part of a scalar token
  // (anything except whitespace or a structural character/'operator').
  json_string_scanner string_scanner{};
  uint64_t still_in_scalar{};
  bitmask::borrow_t still_in_value{};
  uint64_t error{};
};

simdjson_inline uint64_t json_scanner::next(
  const simd::simd8x64<uint8_t>& in
) noexcept {
  // TODO this looks like the same number of SIMD instructions as classifier, but much lower latency
  // uint64_t open_curly    = in.eq('{');   // 2+N (N-1+simd:N)
  // uint64_t open_bracket  = in.eq('[');   // 2+N (N-1+simd:N)
  // uint64_t open          = open_curly | open_bracket;

  // uint64_t close_curly   = in.eq('}');   // 2+N (N-1+simd:N)
  // uint64_t close_bracket = in.eq(']');   // 2+N (N-1+simd:N)
  // uint64_t close         = close_curly | close_bracket;

  // uint64_t comma         = in.eq(',');   // 2+N (N-1+simd:N)
  // uint64_t colon         = in.eq(':');   // 2+N (N-1+simd:N)
  // uint64_t sep           = comma | colon;
  // uint64_t op_without_comma = open | close | colon;

  // uint64_t ws_ctrl       = in.lteq(' '); // 2+N (N-1+simd:N) We will rule out control characters later

  // TODO see if we can use the properties of open/close curly/brackets to reduce these:
  // 0101 1011 [
  // 0101 1101 ]
  // 0111 1011 {
  // 0111 1101 }

  // Classify bytes into comma, open, close, sep, and whitespace.
  simd8x64<uint8_t> classified = CLASSIFIER.classify(in);                           // 9 (+simd:4N)
  uint64_t sep_open         = classified.any_bits_set(SEP | OPEN).to_bitmask();     //   2+N (N-1+simd:N)
  uint64_t scalar_close     = classified.no_bits_set(SEP | OPEN | WS).to_bitmask(); //   2+N (N-1+simd:N)
  uint64_t open_close       = classified.any_bits_set(OPEN | CLOSE).to_bitmask();   //   2+N (N-1+simd:N)
  // critical path = 11+N (3N-3+simd:7N total)

  uint64_t separated_values = next_separated_values(sep_open, scalar_close); // 13 (+2)

  uint64_t backslash = in.eq('\\'); // 2+N (N-1+simd:N)
  uint64_t raw_quote = in.eq('"');  // 2+N (N-1+simd:N)
  uint64_t in_string = string_scanner.next(backslash, raw_quote, separated_values); // 15 (+6) or (13+N or 17+N (+8+simd:3))

  uint64_t comma = in.eq(',');                              // 2+N (N-1+simd:N)
  uint64_t op = sep_open | open_close;                      // 13 (+1)
  uint64_t op_without_comma = op & ~comma;                  //     (ternary)
  uint64_t lead_value = scalar_close & separated_values;    // 16 or 14+N or 18+N (+1)
  uint64_t all_structurals = op_without_comma | lead_value; //    (ternary)
  uint64_t structurals = all_structurals & ~in_string;      // 17 or 15+N or 18+N (+1)
  // critical path = // 17 or 15+N or 18+N (N+3+simd:N)

  uint64_t scalar = scalar_close & open_close;                  // 13 (+1)
  uint64_t sep = sep_open & ~open_close;                        // 13 (+1)
  uint64_t open = sep_open & open_close;                        // 13 (+1)
  check_errors(scalar, sep, open, raw_quote, separated_values, in_string);
  // critical path = [11] 2 (8 total)

  return structurals;
  // structurals: critical path = 17 or 15+N or 18+N (4N+16+simd:8N)
}

simdjson_inline uint64_t json_scanner::next_separated_values(
  uint64_t sep_open,    // 12
  uint64_t scalar_close // 12
) noexcept {
  // Split the JSON by separators. After this, we know:
  // - the lead character of every valid scalar.
  // - there is least one scalar/close bracket between each separator
  // - open bracket is always after separator or at beginning of the document
  // OPEN|WS* CLOSE|SCALAR (CLOSE|SCALAR|WS)* SEP OPEN|WS*
  //    1|0 *      1                   0|1  *  1     1|0 *
  // (We include open brackets with separators because we can easily detect some errors from that.)
  return bitmask::subtract_borrow(sep_open, scalar_close, this->still_in_value);
  // critical path: 13 (+2)
}

simdjson_inline void json_scanner::check_errors(
  uint64_t scalar,           // 13
  uint64_t sep,              // 13
  uint64_t open,             // 13
  uint64_t quote,            //
  uint64_t separated_values,
  uint64_t in_string
) noexcept {
  // Detect separator errors
  // ERROR: missing separator between scalars or close brackets (scalar preceded by anything other than separator, open, or beginning of document)
  uint64_t next_in_scalar = scalar & ~quote;                           // [6]  (ternary &)
  uint64_t in_scalar = next_in_scalar << 1 | still_in_scalar;          //       1 (+1)
  still_in_scalar = next_in_scalar >> 63;
  uint64_t first_scalar = scalar & ~in_scalar;                         //         1
  // Take away lead scalar characters, which are allowed to be the first scalar character
  uint64_t missing_separator_error = first_scalar & ~separated_values; //     (ternary)
  // critical path = [7] 3 (4 total)

  // ERROR: separator with another separator or open bracket ahead of it (or at beginning of document)
  uint64_t extra_separator_error = sep & ~separated_values; // [8] (ternary &)
  // ERROR: open bracket without separator ahead of it (except at beginning of document)
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

simdjson_inline error_code json_scanner::finish() const noexcept {
  if (this->error | this->string_scanner.finish()) {
    return TAPE_ERROR;
  }
}

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_JSON_SCANNER_H