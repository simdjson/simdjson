// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1.h" (this simplifies amalgation)

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

class json_minifier {
public:
  template<size_t STEP_SIZE>
  static error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept;

private:
  simdjson_really_inline json_minifier(uint8_t *_dst)
  : dst{_dst}
  {}
  template<size_t STEP_SIZE>
  simdjson_really_inline void step(const uint8_t *block_buf, buf_block_reader<STEP_SIZE> &reader) noexcept;
  simdjson_really_inline void next(const simd::simd8x64<uint8_t>& in, json_block block);
  simdjson_really_inline error_code finish(uint8_t *dst_start, size_t &dst_len);
  json_scanner scanner{};
  uint8_t *dst;
};

simdjson_really_inline void json_minifier::next(const simd::simd8x64<uint8_t>& in, json_block block) {
  uint64_t mask = block.whitespace();
  in.compress(mask, dst);
  dst += 64 - count_ones(mask);
}

simdjson_really_inline error_code json_minifier::finish(uint8_t *dst_start, size_t &dst_len) {
  error_code error = scanner.finish(false);
  if (error) { dst_len = 0; return error; }
  dst_len = dst - dst_start;
  return SUCCESS;
}

template<>
simdjson_really_inline void json_minifier::step<128>(const uint8_t *block_buf, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  simd::simd8x64<uint8_t> in_2(block_buf+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(in_1, block_1);
  this->next(in_2, block_2);
  reader.advance();
}

template<>
simdjson_really_inline void json_minifier::step<64>(const uint8_t *block_buf, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  json_block block_1 = scanner.next(in_1);
  this->next(block_buf, block_1);
  reader.advance();
}

template<size_t STEP_SIZE>
error_code json_minifier::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept {
  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_minifier minifier(dst);

  // Index the first n-1 blocks
  while (reader.has_full_block()) {
    minifier.step<STEP_SIZE>(reader.full_block(), reader);
  }

  // Index the last (remainder) block, padded with spaces
  uint8_t block[STEP_SIZE];
  size_t remaining_bytes = reader.get_remainder(block); 
  if (remaining_bytes > 0) {
    // We do not want to write directly to the output stream. Rather, we write
    // to a local buffer (for safety).
    uint8_t out_block[STEP_SIZE];
    uint8_t * const guarded_dst{minifier.dst};
    minifier.dst = out_block;
    minifier.step<STEP_SIZE>(block, reader);
    size_t to_write = minifier.dst - out_block;
    // In some cases, we could be enticed to consider the padded spaces
    // as part of the string. This is fine as long as we do not write more
    // than we consumed.
    if(to_write > remaining_bytes) { to_write = remaining_bytes; }
    memcpy(guarded_dst, out_block, to_write);
    minifier.dst = guarded_dst + to_write;
  }
  return minifier.finish(dst, dst_len);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
