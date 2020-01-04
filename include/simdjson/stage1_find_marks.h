#ifndef SIMDJSON_STAGE1_FIND_MARKS_H
#define SIMDJSON_STAGE1_FIND_MARKS_H

#include "simdjson/parsedjson.h"
#include "simdjson/simdjson.h"

namespace simdjson {

// Setting the streaming parameter to true allows the find_structural_bits to tolerate unclosed strings.
// The caller should still ensure that the input is valid UTF-8. If you are processing substrings,
// you may want to call on a function like trimmed_length_safe_utf8.
template <Architecture T = Architecture::NATIVE>
int find_structural_bits(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming = false);

// Setting the streaming parameter to true allows the find_structural_bits to tolerate unclosed strings.
// The caller should still ensure that the input is valid UTF-8. If you are processing substrings,
// you may want to call on a function like trimmed_length_safe_utf8.
template <Architecture T = Architecture::NATIVE>
int find_structural_bits(const char *buf, size_t len, simdjson::ParsedJson &pj, bool streaming = false) {
  return find_structural_bits<T>((const uint8_t *)buf, len, pj, streaming);
}

}; // namespace simdjson

#endif
