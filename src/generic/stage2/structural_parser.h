// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2.h" (this simplifies amalgation)

#include "generic/stage2/tape_writer.h"
#include "generic/stage2/logger.h"
#include "generic/stage2/atomparsing.h"
#include "generic/stage2/structural_iterator.h"

namespace { // Make everything here private
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct structural_parser : structural_iterator {
  /** Lets you append to the tape */
  tape_writer tape;
  /** Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc;
  /** Current depth (nested objects and arrays) */
  uint32_t depth{0};

  // For non-streaming, to pass an explicit 0 as next_structural, which enables optimizations
  really_inline structural_parser(dom_parser_implementation &_parser, uint32_t start_structural_index)
    : structural_iterator(_parser, start_structural_index),
      tape{parser.doc->tape.get()},
      current_string_buf_loc{parser.doc->string_buf.get()} {
  }

  WARN_UNUSED really_inline error_code start_scope(bool is_array) {
    depth++;
    if (depth >= parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
    parser.containing_scope[depth].tape_index = next_tape_index();
    parser.containing_scope[depth].count = 0;
    tape.skip(); // We don't actually *write* the start element until the end.
    parser.is_array[depth] = is_array;
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code start_document() {
    log_start_value("document");
    parser.containing_scope[depth].tape_index = next_tape_index();
    parser.containing_scope[depth].count = 0;
    tape.skip(); // We don't actually *write* the start element until the end.
    parser.is_array[depth] = false;
    if (depth >= parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code start_object() {
    log_start_value("object");
    return start_scope(false);
  }

  WARN_UNUSED really_inline error_code start_array() {
    log_start_value("array");
    return start_scope(true);
  }

  // this function is responsible for annotating the start of the scope
  really_inline void end_scope(internal::tape_type start, internal::tape_type end) noexcept {
    // SIMDJSON_ASSUME(depth > 0);
    // Write the ending tape element, pointing at the start location
    const uint32_t start_tape_index = parser.containing_scope[depth].tape_index;
    tape.append(start_tape_index, end);
    // Write the start tape element, pointing at the end location (and including count)
    // count can overflow if it exceeds 24 bits... so we saturate
    // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
    const uint32_t count = parser.containing_scope[depth].count;
    const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
    tape_writer::write(parser.doc->tape[start_tape_index], next_tape_index() | (uint64_t(cntsat) << 32), start);
    depth--;
  }

  really_inline uint32_t next_tape_index() {
    return uint32_t(tape.next_tape_loc - parser.doc->tape.get());
  }

  really_inline void end_object() {
    log_end_value("object");
    end_scope(internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
  }
  really_inline void end_array() {
    log_end_value("array");
    end_scope(internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
  }
  really_inline void end_document() {
    log_end_value("document");
    constexpr uint32_t start_tape_index = 0;
    tape.append(start_tape_index, internal::tape_type::ROOT);
    tape_writer::write(parser.doc->tape[start_tape_index], next_tape_index(), internal::tape_type::ROOT);
  }

  really_inline void empty_container(internal::tape_type start, internal::tape_type end) {
    auto start_index = next_tape_index();
    tape.append(start_index+2, start);
    tape.append(start_index, end);
  }
  WARN_UNUSED really_inline bool empty_object() {
    if (peek_next_char() == '}') {
      advance_char();
      log_value("empty object");
      empty_container(internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
      return true;
    }
    return false;
  }
  WARN_UNUSED really_inline bool empty_array() {
    if (peek_next_char() == ']') {
      advance_char();
      log_value("empty array");
      empty_container(internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
      return true;
    }
    return false;
  }

  // increment_count increments the count of keys in an object or values in an array.
  really_inline void increment_count() {
    parser.containing_scope[depth].count++; // we have a key value pair in the object at parser.depth - 1
  }

  really_inline uint8_t *on_start_string() noexcept {
    // we advance the point, accounting for the fact that we have a NULL termination
    tape.append(current_string_buf_loc - parser.doc->string_buf.get(), internal::tape_type::STRING);
    return current_string_buf_loc + sizeof(uint32_t);
  }

  really_inline void on_end_string(uint8_t *dst) noexcept {
    uint32_t str_length = uint32_t(dst - (current_string_buf_loc + sizeof(uint32_t)));
    // TODO check for overflow in case someone has a crazy string (>=4GB?)
    // But only add the overflow check when the document itself exceeds 4GB
    // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
    memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
    // NULL termination is still handy if you expect all your strings to
    // be NULL terminated? It comes at a small cost
    *dst = 0;
    current_string_buf_loc = dst + 1;
  }

  WARN_UNUSED really_inline error_code parse_string(bool key = false) {
    log_value(key ? "key" : "string");
    uint8_t *dst = on_start_string();
    dst = stringparsing::parse_string(current(), dst);
    if (dst == nullptr) {
      log_error("Invalid escape in string");
      return STRING_ERROR;
    }
    on_end_string(dst);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_number(const uint8_t *src) {
    log_value("number");
    if (!numberparsing::parse_number(src, tape)) { log_error("Invalid number"); return NUMBER_ERROR; }
    return SUCCESS;
  }
  WARN_UNUSED really_inline error_code parse_number() {
    return parse_number(current());
  }

  really_inline error_code parse_root_number() {
    /**
    * We need to make a copy to make sure that the string is space terminated.
    * This is not about padding the input, which should already padded up
    * to len + SIMDJSON_PADDING. However, we have no control at this stage
    * on how the padding was done. What if the input string was padded with nulls?
    * It is quite common for an input string to have an extra null character (C string).
    * We do not want to allow 9\0 (where \0 is the null character) inside a JSON
    * document, but the string "9\0" by itself is fine. So we make a copy and
    * pad the input with spaces when we know that there is just one input element.
    * This copy is relatively expensive, but it will almost never be called in
    * practice unless you are in the strange scenario where you have many JSON
    * documents made of single atoms.
    */
    uint8_t *copy = static_cast<uint8_t *>(malloc(parser.len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      return MEMALLOC;
    }
    memcpy(copy, buf, parser.len);
    memset(copy + parser.len, ' ', SIMDJSON_PADDING);
    size_t idx = *current_structural;
    error_code error = parse_number(&copy[idx]); // parse_number does not throw
    free(copy);
    return error;
  }

  WARN_UNUSED really_inline error_code parse_true_atom() {
    log_value("true");
    if (!atomparsing::is_valid_true_atom(current())) { return T_ATOM_ERROR; }
    tape.append(0, internal::tape_type::TRUE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_root_true_atom() {
    log_value("true");
    if (!atomparsing::is_valid_true_atom(current(), remaining_len())) { return T_ATOM_ERROR; }
    tape.append(0, internal::tape_type::TRUE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_false_atom() {
    log_value("false");
    if (!atomparsing::is_valid_false_atom(current())) { return F_ATOM_ERROR; }
    tape.append(0, internal::tape_type::FALSE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_root_false_atom() {
    log_value("false");
    if (!atomparsing::is_valid_false_atom(current(), remaining_len())) { return F_ATOM_ERROR; }
    tape.append(0, internal::tape_type::FALSE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_null_atom() {
    log_value("null");
    if (!atomparsing::is_valid_null_atom(current())) { return N_ATOM_ERROR; }
    tape.append(0, internal::tape_type::NULL_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_root_null_atom() {
    log_value("null");
    if (!atomparsing::is_valid_null_atom(current(), remaining_len())) { return N_ATOM_ERROR; }
    tape.append(0, internal::tape_type::NULL_VALUE);
    return SUCCESS;
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
    parser.next_structural_index = uint32_t(current_structural + 1 - &parser.structural_indexes[0]);

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

#define SIMDJSON_TRY(EXPR) { auto _err = (EXPR); if (_err) { return _err; } }

template<bool STREAMING>
WARN_UNUSED static really_inline error_code parse_structurals(dom_parser_implementation &dom_parser, dom::document &doc) noexcept {
  dom_parser.doc = &doc;
  stage2::structural_parser parser(dom_parser, STREAMING ? dom_parser.next_structural_index : 0);
  SIMDJSON_TRY( parser.start() );

  //
  // Read first value
  //
  switch (parser.current_char()) {
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
  case '"': SIMDJSON_TRY( parser.parse_string() ); goto document_end;
  case 't': SIMDJSON_TRY( parser.parse_root_true_atom() ); goto document_end;
  case 'f': SIMDJSON_TRY( parser.parse_root_false_atom() ); goto document_end;
  case 'n': SIMDJSON_TRY( parser.parse_root_null_atom() ); goto document_end;
  case '-':
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    SIMDJSON_TRY( parser.parse_root_number() ); goto document_end;
  default:
    parser.log_error("Document starts with a non-value character");
    return TAPE_ERROR;
  }

//
// Object parser states
//
object_begin:
  if (parser.advance_char() != '"') {
    parser.log_error("Object does not start with a key");
    return TAPE_ERROR;
  }
  parser.increment_count();
  SIMDJSON_TRY( parser.parse_string(true) );
  goto object_field;

object_field:
  if (unlikely( parser.advance_char() != ':' )) { parser.log_error("Missing colon after key in object"); return TAPE_ERROR; }
  switch (parser.advance_char()) {
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
    case '"': SIMDJSON_TRY( parser.parse_string() ); break;
    case 't': SIMDJSON_TRY( parser.parse_true_atom() ); break;
    case 'f': SIMDJSON_TRY( parser.parse_false_atom() ); break;
    case 'n': SIMDJSON_TRY( parser.parse_null_atom() ); break;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      SIMDJSON_TRY( parser.parse_number() ); break;
    default:
      parser.log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }

object_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.increment_count();
    if (unlikely( parser.advance_char() != '"' )) { parser.log_error("Key string missing at beginning of field in object"); return TAPE_ERROR; }
    SIMDJSON_TRY( parser.parse_string(true) );
    goto object_field;
  case '}':
    parser.end_object();
    goto scope_end;
  default:
    parser.log_error("No comma between object fields");
    return TAPE_ERROR;
  }

scope_end:
  if (parser.depth == 0) { goto document_end; }
  if (parser.parser.is_array[parser.depth]) { goto array_continue; }
  goto object_continue;

//
// Array parser states
//
array_begin:
  parser.increment_count();

array_value:
  switch (parser.advance_char()) {
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
    case '"': SIMDJSON_TRY( parser.parse_string() ); break;
    case 't': SIMDJSON_TRY( parser.parse_true_atom() ); break;
    case 'f': SIMDJSON_TRY( parser.parse_false_atom() ); break;
    case 'n': SIMDJSON_TRY( parser.parse_null_atom() ); break;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      SIMDJSON_TRY( parser.parse_number() ); break; 
    default:
      parser.log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }

array_continue:
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

document_end:
  return parser.finish();

} // parse_structurals()

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
