#ifndef SIMDJSON_SRC_GENERIC_STAGE1_BUF_BLOCK_READER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_BUF_BLOCK_READER_H
#include <generic/stage1/base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <cstring>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

// Walks through a buffer in block-sized increments, loading the last part with spaces
template<size_t STEP_SIZE>
struct buf_block_reader {
public:
  simdjson_inline buf_block_reader(const uint8_t *_buf, size_t _len);
  simdjson_inline size_t block_index();
  simdjson_inline bool has_full_block() const;
  simdjson_inline const uint8_t *full_block() const;
  /**
   * Get the last block, padded with spaces.
   *
   * There will always be a last block, with at least 1 byte, unless len == 0 (in which case this
   * function fills the buffer with spaces and returns 0. In particular, if len == STEP_SIZE there
   * will be 0 full_blocks and 1 remainder block with STEP_SIZE bytes and no spaces for padding.
   *
   * @return the number of effective characters in the last block.
   */
  simdjson_inline size_t get_remainder(uint8_t *dst) const;
  simdjson_inline void advance();
private:
  const uint8_t *buf;
  const size_t len;
  const size_t lenminusstep;
  size_t idx;
};

template<size_t STEP_SIZE>
simdjson_inline buf_block_reader<STEP_SIZE>::buf_block_reader(const uint8_t *_buf, size_t _len) : buf{_buf}, len{_len}, lenminusstep{len < STEP_SIZE ? 0 : len - STEP_SIZE}, idx{0} {}

template<size_t STEP_SIZE>
simdjson_inline size_t buf_block_reader<STEP_SIZE>::block_index() { return idx; }

template<size_t STEP_SIZE>
simdjson_inline bool buf_block_reader<STEP_SIZE>::has_full_block() const {
  return idx < lenminusstep;
}

template<size_t STEP_SIZE>
simdjson_inline const uint8_t *buf_block_reader<STEP_SIZE>::full_block() const {
  return &buf[idx];
}

template<size_t STEP_SIZE>
simdjson_inline size_t buf_block_reader<STEP_SIZE>::get_remainder(uint8_t *dst) const {
  if(len == idx) { return 0; } // memcpy(dst, null, 0) will trigger an error with some sanitizers
  std::memset(dst, 0x20, STEP_SIZE); // std::memset STEP_SIZE because it's more efficient to write out 8 or 16 bytes at once.
  std::memcpy(dst, buf + idx, len - idx);
  return len - idx;
}

template<size_t STEP_SIZE>
simdjson_inline void buf_block_reader<STEP_SIZE>::advance() {
  idx += STEP_SIZE;
}

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_BUF_BLOCK_READER_H