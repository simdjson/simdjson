#include "generic/stage2/logger.h"
#include "generic/stage2/structural_iterator.h"
#include "generic/stage2/tape_writer.h"
#include "generic/stage2/atomparsing.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct tape_builder {
  template<bool STREAMING>
  WARN_UNUSED static really_inline error_code parse(dom_parser_implementation &dom_parser) noexcept {
    logger::log_start();
    tape_builder builder(dom_parser);
    SIMDJSON_TRY( builder.iter.walk_document(builder) );
    // If we didn't make it to the end, it's an error
    if ( !STREAMING && dom_parser.next_structural_index != dom_parser.n_structural_indexes ) {
      return builder.error(TAPE_ERROR, "More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
    }
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code root_primitive(const uint8_t *value) {
    switch (*value) {
      case '"': return parse_string(value);
      case 't': return parse_root_true_atom(value);
      case 'f': return parse_root_false_atom(value);
      case 'n': return parse_root_null_atom(value);
      case '-':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        return parse_root_number(value);
      default:
        return error(TAPE_ERROR, "Document starts with a non-value character");
    }
  }
  WARN_UNUSED really_inline error_code primitive(const uint8_t *value) {
    increment_count();
    switch (*value) {
      case '"': return parse_string(value);
      case 't': return parse_true_atom(value);
      case 'f': return parse_false_atom(value);
      case 'n': return parse_null_atom(value);
      case '-':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        return parse_number(value);
      default:
        return error(TAPE_ERROR, "Non-value found when value was expected!");
    }
  }
  WARN_UNUSED really_inline error_code empty_object() {
    increment_count();
    log_value("empty object");
    return empty_container(internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
  }
  WARN_UNUSED really_inline error_code empty_array() {
    increment_count();
    log_value("empty array");
    return empty_container(internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
  }

  WARN_UNUSED really_inline error_code start_document() {
    log_start_value("document");
    start_container();
    iter.dom_parser.is_array[depth] = false;
    return SUCCESS;
  }
  WARN_UNUSED really_inline error_code start_object() {
    increment_count();
    depth++;
    if (depth >= iter.dom_parser.max_depth()) { return DEPTH_ERROR; }
    log_start_value("object");
    start_container();
    iter.dom_parser.is_array[depth] = false;
    return SUCCESS;
  }
  WARN_UNUSED really_inline error_code start_array() {
    increment_count();
    depth++;
    if (depth >= iter.dom_parser.max_depth()) { return DEPTH_ERROR; }
    log_start_value("array");
    start_container();
    iter.dom_parser.is_array[depth] = true;
    return SUCCESS;
  }
  WARN_UNUSED really_inline error_code end_object() {
    log_end_value("object");
    return end_container(internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
  }
  WARN_UNUSED really_inline error_code end_array() {
    log_end_value("array");
    return end_container(internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
  }
  WARN_UNUSED really_inline error_code end_document() {
    log_end_value("document");
    constexpr uint32_t start_tape_index = 0;
    tape.append(start_tape_index, internal::tape_type::ROOT);
    tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(), internal::tape_type::ROOT);
    iter.dom_parser.next_structural_index = uint32_t(iter.next_structural - &iter.dom_parser.structural_indexes[0]);
    if (depth != 0) { return error(TAPE_ERROR, "Unclosed objects or arrays!"); }
    return SUCCESS;
  }
  WARN_UNUSED really_inline error_code try_end_object() {
    SIMDJSON_TRY( try_resume_object() );
    return end_object();
  }
  WARN_UNUSED really_inline error_code try_end_array() {
    SIMDJSON_TRY( try_resume_array() );
    return end_array();
  }
  WARN_UNUSED really_inline error_code try_resume_object() {
    if (depth == 0) { return error(TAPE_ERROR, "Extra values in document"); }
    if (iter.dom_parser.is_array[depth]) { return error(TAPE_ERROR, "Missing key in object field"); }
    return SUCCESS;
  }
  WARN_UNUSED really_inline error_code try_resume_array() {
    if (depth == 0) { return error(TAPE_ERROR, "Extra values in document"); }
    if (!iter.dom_parser.is_array[depth]) { return error(TAPE_ERROR, "Key/value pair in array"); }
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code primitive_field(const uint8_t *key, const uint8_t *value) {
    SIMDJSON_TRY( parse_key(key) );
    return primitive(value);
  }
  WARN_UNUSED really_inline error_code start_object_field(const uint8_t *key) {
    SIMDJSON_TRY( parse_key(key) );
    return start_object();
  }
  WARN_UNUSED really_inline error_code start_array_field(const uint8_t *key) {
    SIMDJSON_TRY( parse_key(key) );
    return start_array();
  }
  WARN_UNUSED really_inline error_code empty_object_field(const uint8_t *key) {
    SIMDJSON_TRY( parse_key(key) );
    return empty_object();
  }
  WARN_UNUSED really_inline error_code empty_array_field(const uint8_t *key) {
    SIMDJSON_TRY( parse_key(key) );
    return empty_array();
  }
  WARN_UNUSED really_inline error_code error(error_code _error, const char *message) {
    log_error(message);
    return _error;
  }

private:
  /** Parser we're using to build the tape */
  structural_iterator iter;
  /** Current depth (nested objects and arrays) */
  uint32_t depth{0};
  /** Next location to write to tape */
  tape_writer tape;
  /** Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc;

  really_inline tape_builder(dom_parser_implementation &dom_parser) noexcept
    : iter(dom_parser, dom_parser.next_structural_index),
      tape{dom_parser.doc->tape.get()},
      current_string_buf_loc{dom_parser.doc->string_buf.get()}
  {
  }

  really_inline void log_value(const char *type) {
    logger::log_line(iter, "", type, "");
  }

  really_inline void log_start_value(const char *type) {
    logger::log_line(iter, "+", type, "");
    if (logger::LOG_ENABLED) { logger::log_depth++; }
  }

  really_inline void log_end_value(const char *type) {
    if (logger::LOG_ENABLED) { logger::log_depth--; }
    logger::log_line(iter, "-", type, "");
  }

  really_inline void log_error(const char *error) {
    logger::log_line(iter, "", "ERROR", error);
  }

  WARN_UNUSED really_inline error_code parse_key(const uint8_t *value) {
    return parse_string(value, true);
  }
  WARN_UNUSED really_inline error_code parse_string(const uint8_t *value, bool key = false) {
    log_value(key ? "key" : "string");
    uint8_t *dst = on_start_string();
    dst = stringparsing::parse_string(value, dst);
    if (dst == nullptr) { return error(STRING_ERROR, "Invalid escape in string"); }
    on_end_string(dst);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_number(const uint8_t *value) {
    log_value("number");
    if (!numberparsing::parse_number(value, tape)) { return error(NUMBER_ERROR, "Invalid number"); }
    return SUCCESS;
  }

  really_inline error_code parse_root_number(const uint8_t *value) {
    //
    // We need to make a copy to make sure that the string is space terminated.
    // This is not about padding the input, which should already padded up
    // to len + SIMDJSON_PADDING. However, we have no control at this stage
    // on how the padding was done. What if the input string was padded with nulls?
    // It is quite common for an input string to have an extra null character (C string).
    // We do not want to allow 9\0 (where \0 is the null character) inside a JSON
    // document, but the string "9\0" by itself is fine. So we make a copy and
    // pad the input with spaces when we know that there is just one input element.
    // This copy is relatively expensive, but it will almost never be called in
    // practice unless you are in the strange scenario where you have many JSON
    // documents made of single atoms.
    //
    uint8_t *copy = static_cast<uint8_t *>(malloc(iter.remaining_len() + SIMDJSON_PADDING));
    if (copy == nullptr) {
      return MEMALLOC;
    }
    memcpy(copy, value, iter.remaining_len());
    memset(copy + iter.remaining_len(), ' ', SIMDJSON_PADDING);
    error_code error = parse_number(copy);
    free(copy);
    return error;
  }

  WARN_UNUSED really_inline error_code parse_true_atom(const uint8_t *value) {
    log_value("true");
    if (!atomparsing::is_valid_true_atom(value)) { return T_ATOM_ERROR; }
    tape.append(0, internal::tape_type::TRUE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_root_true_atom(const uint8_t *value) {
    log_value("true");
    if (!atomparsing::is_valid_true_atom(value, iter.remaining_len())) { return T_ATOM_ERROR; }
    tape.append(0, internal::tape_type::TRUE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_false_atom(const uint8_t *value) {
    log_value("false");
    if (!atomparsing::is_valid_false_atom(value)) { return F_ATOM_ERROR; }
    tape.append(0, internal::tape_type::FALSE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_root_false_atom(const uint8_t *value) {
    log_value("false");
    if (!atomparsing::is_valid_false_atom(value, iter.remaining_len())) { return F_ATOM_ERROR; }
    tape.append(0, internal::tape_type::FALSE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_null_atom(const uint8_t *value) {
    log_value("null");
    if (!atomparsing::is_valid_null_atom(value)) { return N_ATOM_ERROR; }
    tape.append(0, internal::tape_type::NULL_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_root_null_atom(const uint8_t *value) {
    log_value("null");
    if (!atomparsing::is_valid_null_atom(value, iter.remaining_len())) { return N_ATOM_ERROR; }
    tape.append(0, internal::tape_type::NULL_VALUE);
    return SUCCESS;
  }

  // increment_count increments the count of keys in an object or values in an array.
  really_inline void increment_count() {
    iter.dom_parser.open_containers[depth].count++; // we have a key value pair in the object at iter.dom_depth - 1
  }

  really_inline uint32_t next_tape_index() {
    return uint32_t(tape.next_tape_loc - iter.dom_parser.doc->tape.get());
  }

  WARN_UNUSED really_inline error_code empty_container(internal::tape_type start, internal::tape_type end) {
    auto start_index = next_tape_index();
    tape.append(start_index+2, start);
    tape.append(start_index, end);
    return SUCCESS;
  }

  really_inline void start_container() {
    iter.dom_parser.open_containers[depth].tape_index = next_tape_index();
    iter.dom_parser.open_containers[depth].count = 0;
    tape.skip(); // We don't actually *write* the start element until the end.
  }

  WARN_UNUSED really_inline error_code end_container(internal::tape_type start, internal::tape_type end) noexcept {
    // Write the ending tape element, pointing at the start location
    const uint32_t start_tape_index = iter.dom_parser.open_containers[depth].tape_index;
    tape.append(start_tape_index, end);
    // Write the start tape element, pointing at the end location (and including count)
    // count can overflow if it exceeds 24 bits... so we saturate
    // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
    const uint32_t count = iter.dom_parser.open_containers[depth].count;
    const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
    tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index() | (uint64_t(cntsat) << 32), start);
    depth--;
    return SUCCESS;
  }

  really_inline uint8_t *on_start_string() noexcept {
    // we advance the point, accounting for the fact that we have a NULL termination
    tape.append(current_string_buf_loc - iter.dom_parser.doc->string_buf.get(), internal::tape_type::STRING);
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
}; // class tape_builder

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
