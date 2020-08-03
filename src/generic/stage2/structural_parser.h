// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2.h" (this simplifies amalgation)

#include "generic/stage2/logger.h"
#include "generic/stage2/structural_iterator.h"

namespace { // Make everything here private
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

#define SIMDJSON_TRY(EXPR) { auto _err = (EXPR); if (_err) { return _err; } }

struct structural_parser : structural_iterator {
  /** Current depth (nested objects and arrays) */
  uint32_t depth{0};

  template<bool STREAMING, typename T>
  WARN_UNUSED static really_inline error_code parse(dom_parser_implementation &dom_parser, T &builder) noexcept;

  // For non-streaming, to pass an explicit 0 as next_structural, which enables optimizations
  really_inline structural_parser(dom_parser_implementation &_parser, uint32_t start_structural_index)
    : structural_iterator(_parser, start_structural_index) {
  }

  WARN_UNUSED really_inline error_code start_document() {
    parser.is_array[depth] = false;
    return SUCCESS;
  }
  template<typename T>
  WARN_UNUSED really_inline error_code start_object(T &builder) {
    depth++;
    if (depth >= parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
    builder.start_object(*this);
    parser.is_array[depth] = false;
    return SUCCESS;
  }
  template<typename T>
  WARN_UNUSED really_inline error_code start_array(T &builder) {
    depth++;
    if (depth >= parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
    builder.start_array(*this);
    parser.is_array[depth] = true;
    return SUCCESS;
  }

  template<typename T>
  WARN_UNUSED really_inline bool empty_object(T &builder) {
    if (peek_next_char() == '}') {
      advance_char();
      builder.empty_object(*this);
      return true;
    }
    return false;
  }
  template<typename T>
  WARN_UNUSED really_inline bool empty_array(T &builder) {
    if (peek_next_char() == ']') {
      advance_char();
      builder.empty_array(*this);
      return true;
    }
    return false;
  }

  template<bool STREAMING>
  WARN_UNUSED really_inline error_code finish() {
    parser.next_structural_index = uint32_t(next_structural - &parser.structural_indexes[0]);

    if (depth != 0) {
      log_error("Unclosed objects or arrays!");
      return TAPE_ERROR;
    }

    // If we didn't make it to the end, it's an error
    if ( !STREAMING && parser.next_structural_index != parser.n_structural_indexes ) {
      logger::log_string("More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
      return TAPE_ERROR;
    }

    return SUCCESS;
  }

  really_inline void log_value(const char *type) {
    logger::log_line(*this, "", type, "");
  }

  really_inline void log_start_value(const char *type) {
    logger::log_line(*this, "+", type, "");
    if (logger::LOG_ENABLED) { logger::log_depth++; }
  }

  really_inline void log_end_value(const char *type) {
    if (logger::LOG_ENABLED) { logger::log_depth--; }
    logger::log_line(*this, "-", type, "");
  }

  really_inline void log_error(const char *error) {
    logger::log_line(*this, "", "ERROR", error);
  }
}; // struct structural_parser

template<bool STREAMING, typename T>
WARN_UNUSED really_inline error_code structural_parser::parse(dom_parser_implementation &dom_parser, T &builder) noexcept {
  stage2::structural_parser parser(dom_parser, STREAMING ? dom_parser.next_structural_index : 0);
  logger::log_start();

  //
  // Start the document
  //
  if (parser.at_end()) { return EMPTY; }
  SIMDJSON_TRY( parser.start_document() );
  builder.start_document(parser);

  //
  // Read first value
  //
  {
    const uint8_t *value = parser.advance();
    switch (*value) {
    case '{': {
      if (parser.empty_object(builder)) { goto document_end; }
      SIMDJSON_TRY( parser.start_object(builder) );
      goto object_begin;
    }
    case '[': {
      if (parser.empty_array(builder)) { goto document_end; }
      SIMDJSON_TRY( parser.start_array(builder) );
      // Make sure the outer array is closed before continuing; otherwise, there are ways we could get
      // into memory corruption. See https://github.com/simdjson/simdjson/issues/906
      if (!STREAMING) {
        if (parser.buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]] != ']') {
          return TAPE_ERROR;
        }
      }
      goto array_begin;
    }
    case '"': SIMDJSON_TRY( builder.parse_string(parser, value) ); goto document_end;
    case 't': SIMDJSON_TRY( builder.parse_root_true_atom(parser, value) ); goto document_end;
    case 'f': SIMDJSON_TRY( builder.parse_root_false_atom(parser, value) ); goto document_end;
    case 'n': SIMDJSON_TRY( builder.parse_root_null_atom(parser, value) ); goto document_end;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      SIMDJSON_TRY( builder.parse_root_number(parser, value) ); goto document_end;
    default:
      parser.log_error("Document starts with a non-value character");
      return TAPE_ERROR;
    }
  }

//
// Object parser states
//
object_begin: {
  const uint8_t *key = parser.advance();
  if (*key != '"') {
    parser.log_error("Object does not start with a key");
    return TAPE_ERROR;
  }
  builder.increment_count(parser);
  SIMDJSON_TRY( builder.parse_key(parser, key) );
  goto object_field;
} // object_begin:

object_field: {
  if (unlikely( parser.advance_char() != ':' )) { parser.log_error("Missing colon after key in object"); return TAPE_ERROR; }
  const uint8_t *value = parser.advance();
  switch (*value) {
    case '{': {
      if (parser.empty_object(builder)) { break; };
      SIMDJSON_TRY( parser.start_object(builder) );
      goto object_begin;
    }
    case '[': {
      if (parser.empty_array(builder)) { break; };
      SIMDJSON_TRY( parser.start_array(builder) );
      goto array_begin;
    }
    case '"': SIMDJSON_TRY( builder.parse_string(parser, value) ); break;
    case 't': SIMDJSON_TRY( builder.parse_true_atom(parser, value) ); break;
    case 'f': SIMDJSON_TRY( builder.parse_false_atom(parser, value) ); break;
    case 'n': SIMDJSON_TRY( builder.parse_null_atom(parser, value) ); break;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      SIMDJSON_TRY( builder.parse_number(parser, value) ); break;
    default:
      parser.log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }
} // object_field:

object_continue: {
  switch (parser.advance_char()) {
  case ',': {
    builder.increment_count(parser);
    const uint8_t *key = parser.advance();
    if (unlikely( *key != '"' )) { parser.log_error("Key string missing at beginning of field in object"); return TAPE_ERROR; }
    SIMDJSON_TRY( builder.parse_key(parser, key) );
    goto object_field;
  }
  case '}':
    builder.end_object(parser);
    parser.depth--;
    goto scope_end;
  default:
    parser.log_error("No comma between object fields");
    return TAPE_ERROR;
  }
} // object_continue:

scope_end: {
  if (parser.depth == 0) { goto document_end; }
  if (parser.parser.is_array[parser.depth]) { goto array_continue; }
  goto object_continue;
} // scope_end:

//
// Array parser states
//
array_begin: {
  builder.increment_count(parser);
} // array_begin:

array_value: {
  const uint8_t *value = parser.advance();
  switch (*value) {
    case '{': {
      if (parser.empty_object(builder)) { break; };
      SIMDJSON_TRY( parser.start_object(builder) );
      goto object_begin;
    }
    case '[': {
      if (parser.empty_array(builder)) { break; };
      SIMDJSON_TRY( parser.start_array(builder) );
      goto array_begin;
    }
    case '"': SIMDJSON_TRY( builder.parse_string(parser, value) ); break;
    case 't': SIMDJSON_TRY( builder.parse_true_atom(parser, value) ); break;
    case 'f': SIMDJSON_TRY( builder.parse_false_atom(parser, value) ); break;
    case 'n': SIMDJSON_TRY( builder.parse_null_atom(parser, value) ); break;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      SIMDJSON_TRY( builder.parse_number(parser, value) ); break; 
    default:
      parser.log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }
} // array_value:

array_continue: {
  switch (parser.advance_char()) {
  case ',':
    builder.increment_count(parser);
    goto array_value;
  case ']':
    builder.end_array(parser);
    parser.depth--;
    goto scope_end;
  default:
    parser.log_error("Missing comma between array values");
    return TAPE_ERROR;
  }
} // array_continue:

document_end: {
  builder.end_document(parser);
  return parser.finish<STREAMING>();
} // document_end:

} // parse_structurals()

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
