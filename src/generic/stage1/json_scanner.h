#include "simdjson/icelake/bitmask.h"
#ifndef SIMDJSON_SRC_GENERIC_STAGE1_JSON_SCANNER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_JSON_SCANNER_H
#include <generic/stage1/base.h>
#include <generic/stage1/json_string_scanner.h>
#include <generic/stage1/buf_block_reader.h>
#include <simdjson/generic/lookup_table.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

  static simdjson_constinit low_nibble_lookup WHITESPACE_MATCH = {
    { ' ', ' ' },
    { '\t', '\t' },
    { '\r', '\r' },
    { '\n', '\n' },
  };

struct basic_block_classification {
  //printf("\n");
  //printf("%30.30s: %s\n", "next", format_input_text(in));
  uint64_t open;
  uint64_t close;
  uint64_t comma;
  uint64_t colon;
  uint64_t ws_ctrl;
  uint64_t backslash;
  uint64_t raw_quote;
  uint64_t ws;

  simdjson_inline basic_block_classification(const simd8x64<uint8_t>& in) : basic_block_classification(in, in | ('{' - '[')) {}

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

  simdjson_inline basic_block_classification(const simd8x64<uint8_t>& in, const simd8x64<uint8_t>& curlified) :
    open{curlified.eq('{')},
    close{curlified.eq('}')},
    comma{in.eq(',')},
    colon{in.eq(':')},
    ws_ctrl{in.lteq(' ')},
    backslash{in.eq('\\')},
    raw_quote{in.eq('"')},
    ws{in.eq(WHITESPACE_MATCH.lookup(in))}
  {}
};

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
  simdjson_inline json_scanner() = default;
  simdjson_inline uint64_t next(const simd::simd8x64<uint8_t>& in) noexcept;
  simdjson_inline uint64_t next_whitespace(const simd::simd8x64<uint8_t>& in) noexcept;

  // Returns either UNCLOSED_STRING or SUCCESS
  simdjson_inline error_code finish() const noexcept;

  struct input_block {
    uint64_t backslash;
    uint64_t raw_quote;
    uint64_t open;
    uint64_t sep;
    uint64_t sep_open;
    uint64_t scalar_close;
    uint64_t scalar;
    uint64_t op_without_comma;
    uint64_t ctrl;

    simdjson_inline input_block(const basic_block_classification& block) :
      backslash{block.backslash},
      raw_quote{block.raw_quote},
      open{block.open},
      sep{block.comma | block.colon},
      sep_open{sep | open},
      scalar_close{~sep_open & ~block.ws_ctrl},
      scalar{scalar_close & ~block.close},
      op_without_comma{block.colon | open | block.close},
      ctrl{block.ws_ctrl & ~block.ws}
    {}
  };

  struct whitespace_input_block {
    uint64_t backslash;
    uint64_t raw_quote;
    uint64_t ws;
    uint64_t sep_open;
    uint64_t scalar_close;

    simdjson_inline whitespace_input_block(const basic_block_classification& block) :
      backslash{block.backslash},
      raw_quote{block.raw_quote},
      ws{block.ws},
      sep_open{block.comma | block.colon | block.open},
      scalar_close(~sep_open & ~block.ws_ctrl)
    {}
  };

  simdjson_inline uint64_t next(const simd::simd8x64<uint8_t>& in, const input_block& block) noexcept;
  simdjson_inline uint64_t next_whitespace(const simd::simd8x64<uint8_t>& in, const whitespace_input_block& block) noexcept;

private:
  simdjson_inline uint64_t next_separated_values(uint64_t sep_open, uint64_t scalar_close) noexcept;
  simdjson_inline void check_errors(const simd8x64<uint8_t>& in, uint64_t scalar, uint64_t ctrl, uint64_t sep, uint64_t open, uint64_t raw_quote, uint64_t separated_values, uint64_t in_string) noexcept;

  // Whether the last character of the previous iteration is part of a scalar token
  // (anything except whitespace or a structural character/'operator').
  json_string_scanner string_scanner{};
  uint64_t still_in_scalar{};
  bool still_in_value{};
  uint64_t error{};
};

simdjson_inline uint64_t json_scanner::next(const simd::simd8x64<uint8_t>& in) noexcept {
  return next(in, input_block(in));
}

simdjson_inline uint64_t json_scanner::next(const simd::simd8x64<uint8_t>& in, const input_block& block) noexcept {
  //printf("%30.30s: %s\n", "sep_open", format_input_text(in, sep_open));
  //printf("%30.30s: %s\n", "scalar_close", format_input_text(in, scalar_close));
  uint64_t separated_values = next_separated_values(block.sep_open, block.scalar_close); // 8+LN (+2)
  //printf("%30.30s: %s\n", "separated_values", format_input_text(in, separated_values));

  uint64_t quote = string_scanner.next_unescaped_quotes(block.backslash, block.raw_quote);
  uint64_t in_string = string_scanner.next_in_string(quote, separated_values);    // 10+LN (+9) ... 18+LN (+11+simd:3)
  //printf("%30.30s: %s\n", "in_string", format_input_text(in, in_string));
  // total: 11+LN (+10+2LN+simd:2N) ... 19+LN (+18+2LN+simd:2N+3)

  uint64_t lead_value = block.scalar_close & separated_values;    // 8+LN (+1)
  uint64_t all_structurals = block.op_without_comma | lead_value; // 20+LN ... 20+LN (+1)
  // uint64_t op = sep | open | close;                         // 8+LN (+1) (ternary)
  // uint64_t all_structurals = op | lead_value;               // 12+LN ... 20+LN (+1)
  //printf("%30.30s: %s\n", "all_structurals", format_input_text(in, all_structurals));
  uint64_t structurals = all_structurals & ~in_string;      //           (ternary)
  //printf("%30.30s: %s\n", "structurals", format_input_text(in, structurals));
  // critical path = 12+LN ... 20+LN (+3)

  check_errors(in, block.scalar, block.ctrl, block.sep, block.open, quote, separated_values, in_string); // 14+LN (+9)
  // critical path = 14+LN (+11+LN+simd:2N)

  return structurals;
  // structurals: critical path = 12+LN (+25+8LN+simd:10N) ... 20+LN) (+27+8LN+simd:10N+3)
  // = icelake:  12 (24+simd:10) ... 20 (27+simd:13)
  // = haswell:  13 (31+simd:20) ... 21 (35+simd:23)
  // = westmere: 14 (38+simd:40) ... 22 (43+simd:43)
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
  const simd8x64<uint8_t>& ,
  uint64_t scalar,           // 8+LN
  uint64_t ctrl,             // 7+LN
  uint64_t sep,              // 4+LN
  uint64_t open,             // 6+LN
  uint64_t raw_quote,        // 3+LN
  uint64_t separated_values, // 8+LN
  uint64_t in_string         // 12+LN ... 20+LN)
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
  //printf("%30.30s: %s\n", "missing_separator_error", format_input_text(in, missing_separator_error));

  // ERROR: separator with another separator or open bracket ahead of it (or at beginning of document)
  uint64_t extra_separator_error = sep & separated_values;                  // 8+LN  (+1)
  //printf("%30.30s: %s\n", "extra_separator_error", format_input_text(in, extra_separator_error));

  // ERROR: open bracket without separator ahead of it (except at beginning of document)
  uint64_t missing_separator_before_open_error = open & ~separated_values;  // 8+LN  (+1)
  //printf("%30.30s: %s\n", "missing_separator_before_open_error", format_input_text(in, missing_separator_before_open_error));

  //                                                                        // 12+LN (+1) (ternary)
  uint64_t raw_separator_error = missing_separator_error | extra_separator_error | missing_separator_before_open_error;
  //printf("%30.30s: %s\n", "raw_separator_error", format_input_text(in, raw_separator_error));
  // flip lead quote off and trail quote on: lead quote errors
  uint64_t separator_error = raw_separator_error & ~in_string; // 13+LN ... 21+LN (+1) (ternary)
  //printf("%30.30s: %s\n", "separator_error", format_input_text(in, separator_error));
  this->error |= separator_error | ctrl;                                    // 14+LN ... 22+LN (+1) (ternary)
  //printf("%30.30s: %s\n", "error", format_input_text(in, this->error));
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

simdjson_inline uint64_t json_scanner::next_whitespace(
  const simd::simd8x64<uint8_t>& in
) noexcept {
  return next_whitespace(in, basic_block_classification(in));
}

simdjson_inline uint64_t json_scanner::next_whitespace(
  const simd::simd8x64<uint8_t>& in,
  const whitespace_input_block& block
) noexcept {
  uint64_t separated_values = next_separated_values(block.sep_open, block.scalar_close);
  uint64_t in_string = string_scanner.next(block.backslash, block.raw_quote, separated_values);
  return block.ws & ~in_string;
}

simdjson_inline error_code json_scanner::finish() const noexcept {
  if (this->error | this->string_scanner.finish()) {
    return TAPE_ERROR;
  }
  return SUCCESS;
}

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_JSON_SCANNER_H