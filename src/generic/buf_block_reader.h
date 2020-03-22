// Walks through a buffer in block-sized increments, loading the last part with spaces
template<size_t STEP_SIZE>
struct buf_block_reader {
public:
  really_inline buf_block_reader(const uint8_t *_buf, size_t _len) : buf{_buf}, len{_len}, lenminusstep{len < STEP_SIZE ? 0 : len - STEP_SIZE}, idx{0} {}
  really_inline size_t block_index() { return idx; }
  really_inline bool has_full_block() const {
    return idx < lenminusstep;
  }
  really_inline const uint8_t *full_block() const {
    return &buf[idx];
  }
  really_inline bool has_remainder() const {
    return idx < len;
  }
  really_inline void get_remainder(uint8_t *tmp_buf) const {
    memset(tmp_buf, 0x20, STEP_SIZE);
    memcpy(tmp_buf, buf + idx, len - idx);
  }
  really_inline void advance() {
    idx += STEP_SIZE;
  }
private:
  const uint8_t *buf;
  const size_t len;
  const size_t lenminusstep;
  size_t idx;
};

// Routines to print masks and text for debugging bitmask operations
UNUSED static char * format_input_text(const simd8x64<uint8_t> in) {
  static char *buf = (char*)malloc(sizeof(simd8x64<uint8_t>) + 1);
  in.store((uint8_t*)buf);
  for (size_t i=0; i<sizeof(simd8x64<uint8_t>); i++) {
    if (buf[i] < ' ') { buf[i] = '_'; }
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

UNUSED static char * format_mask(uint64_t mask) {
  static char *buf = (char*)malloc(64 + 1);
  for (size_t i=0; i<64; i++) {
    buf[i] = (mask & (size_t(1) << i)) ? 'X' : ' ';
  }
  buf[64] = '\0';
  return buf;
}
