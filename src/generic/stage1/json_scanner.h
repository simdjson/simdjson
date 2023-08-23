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
  simd8x64<uint8_t> curlified = in | ('{' - '[');   // 3 (+simd:N)
  uint64_t open   = curlified.eq('{');              // 6+LN (+LN+simd:N)
  uint64_t close  = curlified.eq('}');              // 6+LN (+LN+simd:N)
  uint64_t comma = in.eq(',');                      // 3+LN (+LN+simd:N)
  uint64_t colon = in.eq(':');                      // 3+LN (+LN+simd:N)
  uint64_t sep   = comma | colon;                   // 4+LN (+1)
  uint64_t ws_ctrl = in.lteq(' ');                  // 3+LN (+LN+simd:N)
  uint64_t sep_open = sep | open;                   // 7+LN (+1)
  uint64_t scalar_close = ~sep & ~open & ~ws_ctrl;  // 7+LN (+1) (ternary)
  // total 7+LN (+3+5LN+simd:6N)

  uint64_t separated_values = next_separated_values(sep_open, scalar_close); // 8+LN (+2)

  uint64_t backslash = in.eq('\\'); // 3+LN (LN+simd:N)
  uint64_t raw_quote = in.eq('"');  // 3+LN (LN+simd:N)
  uint64_t in_string = string_scanner.next(backslash, raw_quote, separated_values); // 10+LN (+6) or (14+LN or 18+LN (+8+simd:3))
  // critical path = 10+LN (+6+2LN+simd:2N) or (14+LN or 18+LN (+8+2LN+simd:2N+3))

  uint64_t lead_value = scalar_close & separated_values;    // 8+LN (+1)
  // uint64_t op_without_comma = colon | open | close;         // 8+LN (+1) (ternary)
  // uint64_t all_structurals = op_without_comma | lead_value; // 11+LN or (15+LN or 19+LN) (+1)
  uint64_t op = sep | open | close;                         // 8+LN (+1) (ternary)
  uint64_t all_structurals = op | lead_value;               // 11+LN or (15+LN or 19+LN) (+1)
  uint64_t structurals = all_structurals & ~in_string;      //           (ternary)
  // critical path = 11+LN or (15+LN or 19+LN) (+3)

  uint64_t scalar = scalar_close & ~close;                  // 8+LN (+1)
  check_errors(scalar, sep, open, raw_quote, separated_values, in_string); // 14+LN (+9)
  // critical path = 14+LN (+10)

  return structurals;
  // structurals: critical path = 11+LN (+24+7LN+simd:8N) or (15+LN or 19+LN) (+26+7LN+simd:8N+3)
  // = icelake:  11 (24+simd:8 ) or (15 or 19) (26+simd:11)
  // = haswell:  12 (31+simd:16) or (16 or 21) (33+simd:19)
  // = westmere: 13 (38+simd:32) or (17 or 22) (40+simd:35)
}

simdjson_inline uint64_t json_scanner::next_separated_values(
  uint64_t sep_open,    // 7+LN
  uint64_t scalar_close // 7+LN
) noexcept {
  // Split the JSON by separators. After this, we know:
  // - the lead character of every valid scalar.
  // - there is least one scalar/close bracket between each separator
  // - open bracket is always after separator or at beginning of the document
  // OPEN|WS* CLOSE|SCALAR (CLOSE|SCALAR|WS)* SEP OPEN|WS*
  //    1|0 *      1                   0|1  *  1     1|0 *
  // (We include open brackets with separators because we can easily detect some errors from that.)
  return bitmask::subtract_borrow(sep_open, scalar_close, this->still_in_value);
  // critical path: 8+LN (+2)
}

simdjson_inline void json_scanner::check_errors(
  uint64_t scalar,           // 8+LN
  uint64_t sep,              // 4+LN
  uint64_t open,             // 6+LN
  uint64_t raw_quote,        // 3+LN
  uint64_t separated_values, // 8+LN
  uint64_t in_string         // 10+LN or (14+LN or 18+LN)
) noexcept {
  // Detect separator errors
  // ERROR: missing separator between scalars or close brackets (scalar preceded by anything other than separator, open, or beginning of document)
  uint64_t next_in_scalar = scalar & ~raw_quote;                            // 8+LN  (ternary)
  uint64_t in_scalar = next_in_scalar << 1 | this->still_in_scalar;         // 10+LN (+2)
  this->still_in_scalar = next_in_scalar >> 63;                             // 9+LN  (+1)
  uint64_t first_scalar = scalar & ~in_scalar;                              // 11+LN (+1)
  // Take away lead scalar characters, which are allowed to be the first scalar character
  uint64_t missing_separator_error = first_scalar & ~separated_values;      //       (ternary)
  // critical path = 11+LN (+4)

  // ERROR: separator with another separator or open bracket ahead of it (or at beginning of document)
  uint64_t extra_separator_error = sep & ~separated_values;                 // 8+LN  (+1)

  // ERROR: open bracket without separator ahead of it (except at beginning of document)
  uint64_t missing_separator_before_open_error = open & separated_values;   // 8+LN  (+1)

  //                                                                        // 12+LN (+1) (ternary)
  uint64_t raw_separator_error = missing_separator_error | extra_separator_error | missing_separator_before_open_error;
  // flip lead quote off and trail quote on: lead quote errors
  uint64_t separator_error = raw_separator_error & (in_string ^ raw_quote); // 13+LN (+1) (ternary)
  this->error |= separator_error;                                           // 14+LN (+1)
  // critical path = 14+LN (+3)

  // NOT validated:
  // - Object/array: Brace balance / type
  // - Object: key type = string
  // - Object: Colon only between key and value
  // - Empty object/array: close bracket before separator preceded by open bracket
  // - UTF-8 in strings
  // - scalar format

  // critical path = 14+LN (+9)
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