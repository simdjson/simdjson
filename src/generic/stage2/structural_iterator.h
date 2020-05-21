namespace stage2 {

class structural_iterator {
public:
  really_inline structural_iterator(parser &_doc_parser, size_t _len)
    : doc_parser{_doc_parser},
      buf{_doc_parser.parsing_buf},
      len{_len},
      structural_indexes{_doc_parser.structural_indexes.get()}
    {}
  really_inline char advance_char() {
    idx = structural_indexes[doc_parser.next_structural];
    doc_parser.next_structural++;
    c = *current();
    return c;
  }
  really_inline char current_char() {
    return c;
  }
  really_inline char peek_char() {
    return buf[structural_indexes[doc_parser.next_structural]];
  }
  really_inline const uint8_t* current() {
    return &buf[idx];
  }
  really_inline size_t remaining_len() {
    return len - idx;
  }
  template<typename F>
  really_inline bool with_space_terminated_copy(const F& f) {
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
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      return true;
    }
    memcpy(copy, buf, len);
    memset(copy + len, ' ', SIMDJSON_PADDING);
    bool result = f(reinterpret_cast<const uint8_t*>(copy), idx);
    free(copy);
    return result;
  }
  really_inline bool past_end(uint32_t n_structural_indexes) {
    return doc_parser.next_structural+1 > n_structural_indexes;
  }
  really_inline bool at_end(uint32_t n_structural_indexes) {
    return doc_parser.next_structural+1 == n_structural_indexes;
  }
  really_inline bool at_beginning() {
    return doc_parser.next_structural == 0;
  }
  really_inline size_t next_structural_index() {
    return doc_parser.next_structural;
  }

  parser &doc_parser;
  const uint8_t* const buf;
  const size_t len;
  const uint32_t* const structural_indexes;
  size_t idx{0}; // location of the structural character in the input (buf)
  uint8_t c{0};  // used to track the (structural) character we are looking at
};

} // namespace stage2
