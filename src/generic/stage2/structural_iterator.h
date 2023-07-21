#ifndef SIMDJSON_SRC_GENERIC_STAGE2_STRUCTURAL_ITERATOR_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE2_STRUCTURAL_ITERATOR_H
#include <generic/stage2/base.h>
#include <simdjson/generic/dom_parser_implementation.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage2 {

class structural_iterator {
public:
  const uint8_t* const buf;
  uint32_t *next_structural;
  dom_parser_implementation &dom_parser;

  // Start a structural
  simdjson_inline structural_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index)
    : buf{_dom_parser.buf},
      next_structural{&_dom_parser.structural_indexes[start_structural_index]},
      dom_parser{_dom_parser} {
  }
  // Get the buffer position of the current structural character
  simdjson_inline const uint8_t* current() {
    return &buf[*(next_structural-1)];
  }
  // Get the current structural character
  simdjson_inline char current_char() {
    return buf[*(next_structural-1)];
  }
  // Get the next structural character without advancing
  simdjson_inline char peek_next_char() {
    return buf[*next_structural];
  }
  simdjson_inline const uint8_t* peek() {
    return &buf[*next_structural];
  }
  simdjson_inline const uint8_t* advance() {
    return &buf[*(next_structural++)];
  }
  simdjson_inline char advance_char() {
    return buf[*(next_structural++)];
  }
  simdjson_inline size_t remaining_len() {
    return dom_parser.len - *(next_structural-1);
  }

  simdjson_inline bool at_end() {
    return next_structural == &dom_parser.structural_indexes[dom_parser.n_structural_indexes];
  }
  simdjson_inline bool at_beginning() {
    return next_structural == dom_parser.structural_indexes.get();
  }
};

} // namespace stage2
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE2_STRUCTURAL_ITERATOR_H