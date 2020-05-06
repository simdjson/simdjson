#ifndef SIMDJSON_DOCUMENT_PARSER_CALLBACKS_H
#define SIMDJSON_DOCUMENT_PARSER_CALLBACKS_H

#include "simdjson.h"

namespace simdjson {
namespace dom {

//
// Parser callbacks
//

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

} // namespace simdjson
} // namespace dom

#endif // SIMDJSON_DOCUMENT_PARSER_CALLBACKS_H
