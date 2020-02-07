#ifndef SIMDJSON_STAGE1_FIND_MARKS_H
#define SIMDJSON_STAGE1_FIND_MARKS_H

#include "simdjson/parsedjson.h"
#include "simdjson/simdjson.h"

namespace simdjson {

// Setting the streaming parameter to true allows the find_structural_bits to tolerate unclosed strings.
// The caller should still ensure that the input is valid UTF-8. If you are processing substrings,
// you may want to call on a function like trimmed_length_safe_utf8.
// A function like find_last_json_buf_idx may also prove useful.
template <architecture T = architecture::NATIVE>
int find_structural_bits(const uint8_t *buf, size_t len, document::parser &parser, bool streaming);

// Setting the streaming parameter to true allows the find_structural_bits to tolerate unclosed strings.
// The caller should still ensure that the input is valid UTF-8. If you are processing substrings,
// you may want to call on a function like trimmed_length_safe_utf8.
// A function like find_last_json_buf_idx may also prove useful.
template <architecture T = architecture::NATIVE>
int find_structural_bits(const char *buf, size_t len, document::parser &parser, bool streaming) {
  return find_structural_bits<T>((const uint8_t *)buf, len, parser, streaming);
}



template <architecture T = architecture::NATIVE>
int find_structural_bits(const uint8_t *buf, size_t len, document::parser &parser) {
     return find_structural_bits<T>(buf, len, parser, false);
}

template <architecture T = architecture::NATIVE>
int find_structural_bits(const char *buf, size_t len, document::parser &parser) {
  return find_structural_bits<T>((const uint8_t *)buf, len, parser);
}

} // namespace simdjson

#endif
