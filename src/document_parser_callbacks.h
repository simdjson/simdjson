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
  /* We do nothing, the actual work occurs in on_end_string() */
  return current_string_buf_loc;
}

bool parser::handle_long_string(uint8_t *dst) noexcept {
  uint64_t str_length = dst - current_string_buf_loc;
  // Oh gosh, we have a long string (8MB). We expect that this is
  // highly uncommon. We want to keep everything else super efficient,
  // so we will pay a complexity price for this one uncommon case.
  uint64_t position = dst - doc.string_buf.get(); // note that we point at the end of the string!
  uint64_t lenmark = uint64_t(0x800000 | (str_length >> 9));
  uint64_t payload =  position |  (lenmark << 32);
  write_tape(payload, internal::tape_type::STRING);
  // We have three free bytes, but
  // we need a leading 1, so that's 24-1 = 23. 32-23=9 remaining bits.
  // We have 9 bits left to code, which we do on the string buffer
  // using two bytes. We encoding the binary data using ASCII characters.
  // See https://lemire.me/blog/2020/05/02/encoding-binary-in-ascii-very-fast/
  // for a more general approach.
  dst[0] = uint8_t(32 + ((str_length & 0x1f0) >> 4)); // (0x1f0>>4)+32 = 63
  dst[1] = uint8_t(32 + (str_length & 0xf)); // 32 + 0xf = 47
  current_string_buf_loc = dst + 2;
  return true;
}

really_inline bool parser::on_end_string(uint8_t *dst) noexcept {
  uint64_t str_length = dst - current_string_buf_loc;
  if(unlikely(str_length > 0x7fffff)) {
    return handle_long_string(dst);
  }
  uint64_t position = current_string_buf_loc - doc.string_buf.get();
  // Long document support: Currently, simdjson supports only document
  // up to 4GB.
  // Should we change this constraint, we should then check for overflow in case
  // someone has a crazy string (>=4GB?).
  write_tape(position | (str_length << 32), internal::tape_type::STRING);
  current_string_buf_loc = dst;
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
