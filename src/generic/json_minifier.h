// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

namespace stage1 {

class json_minifier {
public:
  template<size_t STEP_SIZE>
  static error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept;

private:
  really_inline json_minifier(uint8_t *_dst) : dst{_dst} {}
  template<size_t STEP_SIZE>
  really_inline void step(const uint8_t *block_buf, buf_block_reader<STEP_SIZE> &reader) noexcept;
  really_inline void next(const uint8_t *block_buf, json_block block);
  really_inline error_code finish(uint8_t *dst_start, size_t &dst_len);
  json_scanner scanner;
  uint8_t *dst;
};

struct bitmask_region_iterator {
  uint64_t bitmask;

  struct region {
    int start;
    int end;
    really_inline int len() { return end - start; }
  };

  really_inline bool has_next() { return bitmask; }
  really_inline region next();
};

really_inline bitmask_region_iterator::region bitmask_region_iterator::next()  {
  int start = trailing_zeroes(bitmask);
  // Find the end: adding 1 << start will clear all the ones, setting the next bit after the run to 1.
  bitmask += uint64_t(1) << start;
  int end = trailing_zeroes(bitmask);
  // Clear the end bit
  bitmask &= ~(uint64_t(1) << end);
  return { start, end };
}

really_inline void json_minifier::next(const uint8_t *block_buf, json_block block) {
  bitmask_region_iterator copy_mask { ~block.whitespace() };
  while (copy_mask.has_next()) {
    auto copy_region = copy_mask.next();
    memcpy(dst, block_buf+copy_region.start, copy_region.len());
    dst += copy_region.len();
  }
}

really_inline error_code json_minifier::finish(uint8_t *dst_start, size_t &dst_len) {
  *dst = '\0';
  error_code error = scanner.finish(false);
  if (error) { dst_len = 0; return error; }
  dst_len = dst - dst_start;
  return SUCCESS;
}

template<>
really_inline void json_minifier::step<128>(const uint8_t *block_buf, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  simd::simd8x64<uint8_t> in_2(block_buf+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(block_buf, block_1);
  this->next(block_buf+64, block_2);
  reader.advance();
}

template<>
really_inline void json_minifier::step<64>(const uint8_t *block_buf, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  json_block block_1 = scanner.next(in_1);
  this->next(block_buf, block_1);
  reader.advance();
}

template<size_t STEP_SIZE>
error_code json_minifier::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept {
  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_minifier minifier(dst);
  while (reader.has_full_block()) {
    minifier.step<STEP_SIZE>(reader.full_block(), reader);
  }

  if (likely(reader.has_remainder())) {
    uint8_t block[STEP_SIZE];
    reader.get_remainder(block);
    minifier.step<STEP_SIZE>(block, reader);
  }

  return minifier.finish(dst, dst_len);
}

} // namespace stage1