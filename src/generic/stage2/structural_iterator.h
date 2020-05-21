namespace stage2 {

class structural_iterator {
public:
  really_inline structural_iterator(parser &_doc_parser)
    : doc_parser{_doc_parser},
      buf{_doc_parser.parsing_buf},
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
  really_inline bool past_end() {
    return doc_parser.next_structural+1 > doc_parser.n_structural_indexes;
  }
  really_inline bool at_end() {
    return doc_parser.next_structural+1 == doc_parser.n_structural_indexes;
  }
  really_inline bool at_beginning() {
    return doc_parser.next_structural == 0;
  }
  really_inline size_t next_structural_index() {
    return doc_parser.next_structural;
  }

  parser &doc_parser;
  const uint8_t* const buf;
  const uint32_t* const structural_indexes;
  size_t idx{0}; // location of the structural character in the input (buf)
  uint8_t c{0};  // used to track the (structural) character we are looking at
};

} // namespace stage2
