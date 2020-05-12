namespace stage2 {

class structural_iterator {
public:
  really_inline structural_iterator(const uint8_t* _buf, size_t _len, const uint32_t *_structural_indexes, size_t next_structural_index)
    : buf{_buf},
     len{_len},
     structural_indexes{_structural_indexes},
     next_structural{next_structural_index}
    {}
  really_inline char advance_char() {
    idx = structural_indexes[next_structural];
    next_structural++;
    c = *current();
    return c;
  }
  really_inline char current_char() {
    return c;
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
    return next_structural+1 > n_structural_indexes;
  }
  really_inline bool at_end(uint32_t n_structural_indexes) {
    return next_structural+1 == n_structural_indexes;
  }
  really_inline size_t next_structural_index() {
    return next_structural;
  }

  const uint8_t* const buf;
  const size_t len;
  const uint32_t* const structural_indexes;
  size_t next_structural; // next structural index
  size_t idx{0}; // location of the structural character in the input (buf)
  uint8_t c{0};  // used to track the (structural) character we are looking at
};

} // namespace stage2
