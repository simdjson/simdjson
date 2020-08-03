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

template<typename T>
struct structural_parser : structural_iterator {
  /** Receiver that actually parses the strings and builds the tape */
  T builder;
  /** Current depth (nested objects and arrays) */
  uint32_t depth{0};

  // For non-streaming, to pass an explicit 0 as next_structural, which enables optimizations
  really_inline structural_parser(dom_parser_implementation &_parser, uint32_t start_structural_index)
    : structural_iterator(_parser, start_structural_index),
      builder{parser.doc->tape.get(), parser.doc->string_buf.get()} {
  }

  WARN_UNUSED really_inline error_code start_document() {
    builder.start_document(*this);
    parser.is_array[depth] = false;
    return SUCCESS;
  }
  WARN_UNUSED really_inline error_code start_object() {
    depth++;
    if (depth >= parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
    builder.start_object(*this);
    parser.is_array[depth] = false;
    return SUCCESS;
  }
  WARN_UNUSED really_inline error_code start_array() {
    depth++;
    if (depth >= parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
    builder.start_array(*this);
    parser.is_array[depth] = true;
    return SUCCESS;
  }
  really_inline void end_object() {
    builder.end_object(*this);
    depth--;
  }
  really_inline void end_array() {
    builder.end_array(*this);
    depth--;
  }
  really_inline void end_document() {
    builder.end_document(*this);
  }

  WARN_UNUSED really_inline bool empty_object() {
    if (peek_next_char() == '}') {
      advance_char();
      builder.empty_object(*this);
      return true;
    }
    return false;
  }
  WARN_UNUSED really_inline bool empty_array() {
    if (peek_next_char() == ']') {
      advance_char();
      builder.empty_array(*this);
      return true;
    }
    return false;
  }

  really_inline void increment_count() {
    builder.increment_count(*this);
  }

  WARN_UNUSED really_inline error_code parse_key(const uint8_t *key) {
    return builder.parse_key(*this, key);
  }
  WARN_UNUSED really_inline error_code parse_string(const uint8_t *value) {
    return builder.parse_string(*this, value);
  }
  WARN_UNUSED really_inline error_code parse_number(const uint8_t *value) {
    return builder.parse_number(*this, value);
  }
  WARN_UNUSED really_inline error_code parse_root_number(const uint8_t *value) {
    return builder.parse_root_number(*this, value);
  }
  WARN_UNUSED really_inline error_code parse_true_atom(const uint8_t *value) {
    return builder.parse_true_atom(*this, value);
  }
  WARN_UNUSED really_inline error_code parse_root_true_atom(const uint8_t *value) {
    return builder.parse_root_true_atom(*this, value);
  }
  WARN_UNUSED really_inline error_code parse_false_atom(const uint8_t *value) {
    return builder.parse_false_atom(*this, value);
  }
  WARN_UNUSED really_inline error_code parse_root_false_atom(const uint8_t *value) {
    return builder.parse_root_false_atom(*this, value);
  }
  WARN_UNUSED really_inline error_code parse_null_atom(const uint8_t *value) {
    return builder.parse_null_atom(*this, value);
  }
  WARN_UNUSED really_inline error_code parse_root_null_atom(const uint8_t *value) {
    return builder.parse_root_null_atom(*this, value);
  }

  WARN_UNUSED really_inline error_code start() {
    logger::log_start();

    // If there are no structurals left, return EMPTY
    if (at_end()) { return EMPTY; }

    // Push the root scope (there is always at least one scope)
    return start_document();
  }

  WARN_UNUSED really_inline error_code finish() {
    end_document();
    parser.next_structural_index = uint32_t(next_structural - &parser.structural_indexes[0]);

    if (depth != 0) {
      log_error("Unclosed objects or arrays!");
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

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#include "generic/stage2/tape_builder.h"

namespace { // Make everything here private
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

template<bool STREAMING>
WARN_UNUSED static really_inline error_code parse_structurals(dom_parser_implementation &dom_parser, dom::document &doc) noexcept {
  dom_parser.doc = &doc;
  stage2::structural_parser<stage2::tape_builder> parser(dom_parser, STREAMING ? dom_parser.next_structural_index : 0);
  SIMDJSON_TRY( parser.start() );

  //
  // Read first value
  //
  {
    const uint8_t *value = parser.advance();
    switch (*value) {
    case '{': {
      if (parser.empty_object()) { goto document_end; }
      SIMDJSON_TRY( parser.start_object() );
      goto object_begin;
    }
    case '[': {
      if (parser.empty_array()) { goto document_end; }
      SIMDJSON_TRY( parser.start_array() );
      // Make sure the outer array is closed before continuing; otherwise, there are ways we could get
      // into memory corruption. See https://github.com/simdjson/simdjson/issues/906
      if (!STREAMING) {
        if (parser.buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]] != ']') {
          return TAPE_ERROR;
        }
      }
      goto array_begin;
    }
    case '"': SIMDJSON_TRY( parser.parse_string(value) ); goto document_end;
    case 't': SIMDJSON_TRY( parser.parse_root_true_atom(value) ); goto document_end;
    case 'f': SIMDJSON_TRY( parser.parse_root_false_atom(value) ); goto document_end;
    case 'n': SIMDJSON_TRY( parser.parse_root_null_atom(value) ); goto document_end;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      SIMDJSON_TRY( parser.parse_root_number(value) ); goto document_end;
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
  parser.increment_count();
  SIMDJSON_TRY( parser.parse_key(key) );
  goto object_field;
} // object_begin:

object_field: {
  if (unlikely( parser.advance_char() != ':' )) { parser.log_error("Missing colon after key in object"); return TAPE_ERROR; }
  const uint8_t *value = parser.advance();
  switch (*value) {
    case '{': {
      if (parser.empty_object()) { break; };
      SIMDJSON_TRY( parser.start_object() );
      goto object_begin;
    }
    case '[': {
      if (parser.empty_array()) { break; };
      SIMDJSON_TRY( parser.start_array() );
      goto array_begin;
    }
    case '"': SIMDJSON_TRY( parser.parse_string(value) ); break;
    case 't': SIMDJSON_TRY( parser.parse_true_atom(value) ); break;
    case 'f': SIMDJSON_TRY( parser.parse_false_atom(value) ); break;
    case 'n': SIMDJSON_TRY( parser.parse_null_atom(value) ); break;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      SIMDJSON_TRY( parser.parse_number(value) ); break;
    default:
      parser.log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }
} // object_field:

object_continue: {
  switch (parser.advance_char()) {
  case ',': {
    parser.increment_count();
    const uint8_t *key = parser.advance();
    if (unlikely( *key != '"' )) { parser.log_error("Key string missing at beginning of field in object"); return TAPE_ERROR; }
    SIMDJSON_TRY( parser.parse_key(key) );
    goto object_field;
  }
  case '}':
    parser.end_object();
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
  parser.increment_count();
} // array_begin:

array_value: {
  const uint8_t *value = parser.advance();
  switch (*value) {
    case '{': {
      if (parser.empty_object()) { break; };
      SIMDJSON_TRY( parser.start_object() );
      goto object_begin;
    }
    case '[': {
      if (parser.empty_array()) { break; };
      SIMDJSON_TRY( parser.start_array() );
      goto array_begin;
    }
    case '"': SIMDJSON_TRY( parser.parse_string(value) ); break;
    case 't': SIMDJSON_TRY( parser.parse_true_atom(value) ); break;
    case 'f': SIMDJSON_TRY( parser.parse_false_atom(value) ); break;
    case 'n': SIMDJSON_TRY( parser.parse_null_atom(value) ); break;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      SIMDJSON_TRY( parser.parse_number(value) ); break; 
    default:
      parser.log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }
} // array_value:

array_continue: {
  switch (parser.advance_char()) {
  case ',':
    parser.increment_count();
    goto array_value;
  case ']':
    parser.end_array();
    goto scope_end;
  default:
    parser.log_error("Missing comma between array values");
    return TAPE_ERROR;
  }
} // array_continue:

document_end: {
  return parser.finish();
} // document_end:

} // parse_structurals()

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
