#ifndef SIMDJSON_DOCUMENT_PARSER_CALLBACKS_H
#define SIMDJSON_DOCUMENT_PARSER_CALLBACKS_H

#include "simdjson.h"

namespace simdjson {
namespace dom {

//
// Parser callbacks
//

inline void parser::init_stage2() noexcept {
  current_string_buf_loc = doc.string_buf.get();
  current_loc = 0;
  valid = false;
  error = UNINITIALIZED;
}

really_inline error_code parser::on_error(error_code new_error_code) noexcept {
  error = new_error_code;
  return new_error_code;
}
really_inline error_code parser::on_success(error_code success_code) noexcept {
  error = success_code;
  valid = true;
  return success_code;
}
// increment_count increments the count of keys in an object or values in an array.
// Note that if you are at the level of the values or elements, the count
// must be increment in the preceding depth (depth-1) where the array or
// the object resides.
really_inline void parser::increment_count(uint32_t depth) noexcept {
  containing_scope[depth].count++;
}

really_inline bool parser::on_start_document(uint32_t depth) noexcept {
  containing_scope[depth].tape_index = current_loc;
  containing_scope[depth].count = 0;
  write_tape(0, internal::tape_type::ROOT); // if the document is correct, this gets rewritten later
  return true;
}
really_inline bool parser::on_start_object(uint32_t depth) noexcept {
  containing_scope[depth].tape_index = current_loc;
  containing_scope[depth].count = 0;
  write_tape(0, internal::tape_type::START_OBJECT);  // if the document is correct, this gets rewritten later
  return true;
}
really_inline bool parser::on_start_array(uint32_t depth) noexcept {
  containing_scope[depth].tape_index = current_loc;
  containing_scope[depth].count = 0;
  write_tape(0, internal::tape_type::START_ARRAY);  // if the document is correct, this gets rewritten later
  return true;
}
// TODO we're not checking this bool
really_inline bool parser::on_end_document(uint32_t depth) noexcept {
  // write our doc.tape location to the header scope
  // The root scope gets written *at* the previous location.
  write_tape(containing_scope[depth].tape_index, internal::tape_type::ROOT);
  end_scope(depth);
  return true;
}
really_inline bool parser::on_end_object(uint32_t depth) noexcept {
  // write our doc.tape location to the header scope
  write_tape(containing_scope[depth].tape_index, internal::tape_type::END_OBJECT);
  end_scope(depth);
  return true;
}
really_inline bool parser::on_end_array(uint32_t depth) noexcept {
  // write our doc.tape location to the header scope
  write_tape(containing_scope[depth].tape_index, internal::tape_type::END_ARRAY);
  end_scope(depth);
  return true;
}

really_inline bool parser::on_true_atom() noexcept {
  write_tape(0, internal::tape_type::TRUE_VALUE);
  return true;
}
really_inline bool parser::on_false_atom() noexcept {
  write_tape(0, internal::tape_type::FALSE_VALUE);
  return true;
}
really_inline bool parser::on_null_atom() noexcept {
  write_tape(0, internal::tape_type::NULL_VALUE);
  return true;
}

really_inline uint8_t *parser::on_start_string() noexcept {
  /* we advance the point, accounting for the fact that we have a NULL
    * termination         */
  write_tape(current_string_buf_loc - doc.string_buf.get(), internal::tape_type::STRING);
  return current_string_buf_loc + sizeof(uint32_t);
}

really_inline bool parser::on_end_string(uint8_t *dst) noexcept {
  uint32_t str_length = uint32_t(dst - (current_string_buf_loc + sizeof(uint32_t)));
  // TODO check for overflow in case someone has a crazy string (>=4GB?)
  // But only add the overflow check when the document itself exceeds 4GB
  // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
  memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
  // NULL termination is still handy if you expect all your strings to
  // be NULL terminated? It comes at a small cost
  *dst = 0;
  current_string_buf_loc = dst + 1;
  return true;
}

really_inline bool parser::on_number_s64(int64_t value) noexcept {
  write_tape(0, internal::tape_type::INT64);
  std::memcpy(&doc.tape[current_loc], &value, sizeof(value));
  ++current_loc;
  return true;
}
really_inline bool parser::on_number_u64(uint64_t value) noexcept {
  write_tape(0, internal::tape_type::UINT64);
  doc.tape[current_loc++] = value;
  return true;
}
really_inline bool parser::on_number_double(double value) noexcept {
  write_tape(0, internal::tape_type::DOUBLE);
  static_assert(sizeof(value) == sizeof(doc.tape[current_loc]), "mismatch size");
  memcpy(&doc.tape[current_loc++], &value, sizeof(double));
  // doc.tape[doc.current_loc++] = *((uint64_t *)&d);
  return true;
}

really_inline void parser::write_tape(uint64_t val, internal::tape_type t) noexcept {
  doc.tape[current_loc++] = val | ((uint64_t(char(t))) << 56);
}

// this function is responsible for annotating the start of the scope
really_inline void parser::end_scope(uint32_t depth) noexcept {
  scope_descriptor d = containing_scope[depth];
  // count can overflow if it exceeds 24 bits... so we saturate
  // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
  const uint32_t cntsat =  d.count > 0xFFFFFF ? 0xFFFFFF : d.count;
  // This is a load and an OR. It would be possible to just write once at doc.tape[d.tape_index]
  doc.tape[d.tape_index] |= current_loc | (uint64_t(cntsat) << 32);
}

} // namespace simdjson
} // namespace dom

#endif // SIMDJSON_DOCUMENT_PARSER_CALLBACKS_H
