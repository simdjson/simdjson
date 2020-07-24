namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage2 {

class structural_iterator {
public:
  const uint8_t* const buf;
  uint32_t *current_structural;
  dom_parser_implementation &parser;

  // Start a structural 
  really_inline structural_iterator(dom_parser_implementation &_parser, size_t start_structural_index)
    : buf{_parser.buf},
      current_structural{&_parser.structural_indexes[start_structural_index]},
      parser{_parser} {
  }
  // Get the buffer position of the current structural character
  really_inline const uint8_t* current() {
    return &buf[*current_structural];
  }
  // Get the current structural character
  really_inline char current_char() {
    return buf[*current_structural];
  }
  // Get the next structural character without advancing
  really_inline char peek_next_char() {
    return buf[*(current_structural+1)];
  }
  really_inline char advance_char() {
    current_structural++;
    return buf[*current_structural];
  }
  really_inline size_t remaining_len() {
    return parser.len - *current_structural;
  }

  really_inline bool past_end(uint32_t n_structural_indexes) {
    return current_structural >= &parser.structural_indexes[n_structural_indexes];
  }
  really_inline bool at_end(uint32_t n_structural_indexes) {
    return current_structural == &parser.structural_indexes[n_structural_indexes];
  }
  really_inline bool at_beginning() {
    return current_structural == parser.structural_indexes.get();
  }
};

} // namespace stage2
} // namespace {
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
