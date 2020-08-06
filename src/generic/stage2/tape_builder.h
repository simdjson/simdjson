#include "generic/stage2/structural_parser.h"
#include "generic/stage2/tape_writer.h"
#include "generic/stage2/atomparsing.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct tape_builder {
  /** Next location to write to tape */
  tape_writer tape;
  /** Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc;

  template<bool STREAMING>
  WARN_UNUSED static really_inline error_code parse_document(
      dom_parser_implementation &dom_parser,
      dom::document &doc) noexcept {
    dom_parser.doc = &doc;
    structural_parser iter(dom_parser, STREAMING ? dom_parser.next_structural_index : 0);
    tape_builder builder(doc);
    return iter.walk_document<STREAMING>(builder);
  }

private:
  friend struct structural_parser;

  really_inline tape_builder(dom::document &doc) noexcept : tape{doc.tape.get()}, current_string_buf_loc{doc.string_buf.get()} {}

  really_inline error_code parse_root_primitive(structural_parser &iter, const uint8_t *value) {
    switch (*value) {
      case '"': return parse_string(iter, value);
      case 't': return parse_root_true_atom(iter, value);
      case 'f': return parse_root_false_atom(iter, value);
      case 'n': return parse_root_null_atom(iter, value);
      case '-':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        return parse_root_number(iter, value);
      default:
        iter.log_error("Document starts with a non-value character");
        return TAPE_ERROR;
    }
  }
  really_inline error_code parse_primitive(structural_parser &iter, const uint8_t *value) {
    switch (*value) {
      case '"': return parse_string(iter, value);
      case 't': return parse_true_atom(iter, value);
      case 'f': return parse_false_atom(iter, value);
      case 'n': return parse_null_atom(iter, value);
      case '-':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        return parse_number(iter, value);
      default:
        iter.log_error("Non-value found when value was expected!");
        return TAPE_ERROR;
    }
  }
  really_inline void empty_object(structural_parser &iter) {
    iter.log_value("empty object");
    empty_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
  }
  really_inline void empty_array(structural_parser &iter) {
    iter.log_value("empty array");
    empty_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
  }

  really_inline void start_document(structural_parser &iter) {
    iter.log_start_value("document");
    start_container(iter);
    iter.dom_parser.is_array[iter.depth] = false;
  }
  really_inline void start_object(structural_parser &iter) {
    iter.log_start_value("object");
    start_container(iter);
    iter.dom_parser.is_array[iter.depth] = false;
  }
  really_inline void start_array(structural_parser &iter) {
    iter.log_start_value("array");
    start_container(iter);
    iter.dom_parser.is_array[iter.depth] = true;
  }

  really_inline void end_object(structural_parser &iter) {
    iter.log_end_value("object");
    end_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
  }
  really_inline void end_array(structural_parser &iter) {
    iter.log_end_value("array");
    end_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
  }
  really_inline void end_document(structural_parser &iter) {
    iter.log_end_value("document");
    constexpr uint32_t start_tape_index = 0;
    tape.append(start_tape_index, internal::tape_type::ROOT);
    tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter), internal::tape_type::ROOT);
  }

  WARN_UNUSED really_inline error_code parse_key(structural_parser &iter, const uint8_t *value) {
    return parse_string(iter, value, true);
  }
  WARN_UNUSED really_inline error_code parse_string(structural_parser &iter, const uint8_t *value, bool key = false) {
    iter.log_value(key ? "key" : "string");
    uint8_t *dst = on_start_string(iter);
    dst = stringparsing::parse_string(value, dst);
    if (dst == nullptr) {
      iter.log_error("Invalid escape in string");
      return STRING_ERROR;
    }
    on_end_string(dst);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_number(structural_parser &iter, const uint8_t *value) {
    iter.log_value("number");
    if (!numberparsing::parse_number(value, tape)) { iter.log_error("Invalid number"); return NUMBER_ERROR; }
    return SUCCESS;
  }

  really_inline error_code parse_root_number(structural_parser &iter, const uint8_t *value) {
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
    error_code error = parse_number(iter, copy);
    free(copy);
    return error;
  }

  WARN_UNUSED really_inline error_code parse_true_atom(structural_parser &iter, const uint8_t *value) {
    iter.log_value("true");
    if (!atomparsing::is_valid_true_atom(value)) { return T_ATOM_ERROR; }
    tape.append(0, internal::tape_type::TRUE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_root_true_atom(structural_parser &iter, const uint8_t *value) {
    iter.log_value("true");
    if (!atomparsing::is_valid_true_atom(value, iter.remaining_len())) { return T_ATOM_ERROR; }
    tape.append(0, internal::tape_type::TRUE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_false_atom(structural_parser &iter, const uint8_t *value) {
    iter.log_value("false");
    if (!atomparsing::is_valid_false_atom(value)) { return F_ATOM_ERROR; }
    tape.append(0, internal::tape_type::FALSE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_root_false_atom(structural_parser &iter, const uint8_t *value) {
    iter.log_value("false");
    if (!atomparsing::is_valid_false_atom(value, iter.remaining_len())) { return F_ATOM_ERROR; }
    tape.append(0, internal::tape_type::FALSE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_null_atom(structural_parser &iter, const uint8_t *value) {
    iter.log_value("null");
    if (!atomparsing::is_valid_null_atom(value)) { return N_ATOM_ERROR; }
    tape.append(0, internal::tape_type::NULL_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code parse_root_null_atom(structural_parser &iter, const uint8_t *value) {
    iter.log_value("null");
    if (!atomparsing::is_valid_null_atom(value, iter.remaining_len())) { return N_ATOM_ERROR; }
    tape.append(0, internal::tape_type::NULL_VALUE);
    return SUCCESS;
  }

  // increment_count increments the count of keys in an object or values in an array.
  really_inline void increment_count(structural_parser &iter) {
    iter.dom_parser.open_containers[iter.depth].count++; // we have a key value pair in the object at parser.dom_parser.depth - 1
  }

// private:

  really_inline uint32_t next_tape_index(structural_parser &iter) {
    return uint32_t(tape.next_tape_loc - iter.dom_parser.doc->tape.get());
  }

  really_inline void empty_container(structural_parser &iter, internal::tape_type start, internal::tape_type end) {
    auto start_index = next_tape_index(iter);
    tape.append(start_index+2, start);
    tape.append(start_index, end);
  }

  really_inline void start_container(structural_parser &iter) {
    iter.dom_parser.open_containers[iter.depth].tape_index = next_tape_index(iter);
    iter.dom_parser.open_containers[iter.depth].count = 0;
    tape.skip(); // We don't actually *write* the start element until the end.
  }

  really_inline void end_container(structural_parser &iter, internal::tape_type start, internal::tape_type end) noexcept {
    // Write the ending tape element, pointing at the start location
    const uint32_t start_tape_index = iter.dom_parser.open_containers[iter.depth].tape_index;
    tape.append(start_tape_index, end);
    // Write the start tape element, pointing at the end location (and including count)
    // count can overflow if it exceeds 24 bits... so we saturate
    // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
    const uint32_t count = iter.dom_parser.open_containers[iter.depth].count;
    const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
    tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter) | (uint64_t(cntsat) << 32), start);
  }

  really_inline uint8_t *on_start_string(structural_parser &iter) noexcept {
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
