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
  /**
   * Like get_remainder, for input followed by SIMDJSON_PADDING readable bytes.
   *
   * get_remainder writes the block with a memset and a memcpy of the remaining
   * length, and the vector loads that read it back cannot be forwarded from
   * those narrower stores: they wait until the stores reach the cache, which
   * costs tens of cycles on every document. Here the block is put together in
   * vector registers, from loads that may read into the padding, and written
   * with full-width stores that the loads can forward from.
   */
  simdjson_inline size_t get_remainder_padded(uint8_t *dst) const;
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

// keep_mask[128 - n + i] is 0xFF for i < n and 0 otherwise.
alignas(64) static const uint8_t keep_mask[256] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

template<size_t STEP_SIZE>
simdjson_inline size_t buf_block_reader<STEP_SIZE>::get_remainder_padded(uint8_t *dst) const {
  static_assert(STEP_SIZE <= 128, "keep_mask covers blocks of up to 128 bytes");
  static_assert(sizeof(simd::simd8<uint8_t>) <= SIMDJSON_PADDING, "a vector load at the last byte must stay in the padding");
  const size_t remaining = len - idx;
  if (remaining == 0) { return 0; }
  const simd::simd8<uint8_t> spaces = simd::simd8<uint8_t>::splat(0x20);
  for (size_t off = 0; off < STEP_SIZE; off += sizeof(simd::simd8<uint8_t>)) {
    simd::simd8<uint8_t> v = spaces;
    if (off < remaining) {
      // Starts before len, so it ends within the padding.
      const simd::simd8<uint8_t> in = simd::simd8<uint8_t>::load(buf + idx + off);
      const simd::simd8<uint8_t> keep = simd::simd8<uint8_t>::load(keep_mask + 128 - remaining + off);
      v = spaces ^ ((in ^ spaces) & keep);
    }
    v.store(dst + off);
  }
  return remaining;
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