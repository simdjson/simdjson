#ifndef SIMDJSON_SRC_ARM64_CPP
#define SIMDJSON_SRC_ARM64_CPP

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include <base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <simdjson/arm64.h>
#include <simdjson/arm64/implementation.h>

#include <simdjson/arm64/begin.h>
#include <generic/amalgamated.h>
#include <generic/stage1/amalgamated.h>
#include <generic/stage2/amalgamated.h>

//
// Stage 1
//
namespace simdjson {
namespace arm64 {

simdjson_warn_unused error_code implementation::create_dom_parser_implementation(
  size_t capacity,
  size_t max_depth,
  std::unique_ptr<internal::dom_parser_implementation>& dst
) const noexcept {
  dst.reset( new (std::nothrow) dom_parser_implementation() );
  if (!dst) { return MEMALLOC; }
  if (auto err = dst->set_capacity(capacity))
    return err;
  if (auto err = dst->set_max_depth(max_depth))
    return err;
  return SUCCESS;
}

namespace {

using namespace simd;

simdjson_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t>& in) {
  const uint8x16_t op_table = simd8<uint8_t>(
    0xff, 0, ',', ':', 0, '[', ']', '{', '}', 0, 0, 0, 0, 0, 0, 0
  );
  const uint8x16_t ws_table = simd8<uint8_t>(
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0, 0, 0xff, 0, 0
  );

  const uint8x16_t d0_0 = in.chunks[0];
  const uint8x16_t d0_1 = in.chunks[1];
  const uint8x16_t d0_2 = in.chunks[2];
  const uint8x16_t d0_3 = in.chunks[3];

  const uint8x16_t match_op_0 = vceqq_u8(vqtbl1q_u8(op_table, vshrq_n_u8(vaddq_u8(d0_0, vdupq_n_u8(3)), 4)), d0_0);
  const uint8x16_t match_op_1 = vceqq_u8(vqtbl1q_u8(op_table, vshrq_n_u8(vaddq_u8(d0_1, vdupq_n_u8(3)), 4)), d0_1);
  const uint8x16_t match_op_2 = vceqq_u8(vqtbl1q_u8(op_table, vshrq_n_u8(vaddq_u8(d0_2, vdupq_n_u8(3)), 4)), d0_2);
  const uint8x16_t match_op_3 = vceqq_u8(vqtbl1q_u8(op_table, vshrq_n_u8(vaddq_u8(d0_3, vdupq_n_u8(3)), 4)), d0_3);

  const uint8x16_t match_ws_0 = vqtbx1q_u8(vceqq_u8(d0_0, vdupq_n_u8(' ')), ws_table, d0_0);
  const uint8x16_t match_ws_1 = vqtbx1q_u8(vceqq_u8(d0_1, vdupq_n_u8(' ')), ws_table, d0_1);
  const uint8x16_t match_ws_2 = vqtbx1q_u8(vceqq_u8(d0_2, vdupq_n_u8(' ')), ws_table, d0_2);
  const uint8x16_t match_ws_3 = vqtbx1q_u8(vceqq_u8(d0_3, vdupq_n_u8(' ')), ws_table, d0_3);

  const uint8x16_t bit_mask = simd8<uint8_t>(
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
  );

  uint8x16_t op_sum0 = vpaddq_u8(vandq_u8(match_op_0, bit_mask), vandq_u8(match_op_1, bit_mask));
  uint8x16_t ws_sum0 = vpaddq_u8(vandq_u8(match_ws_0, bit_mask), vandq_u8(match_ws_1, bit_mask));
  uint8x16_t op_sum1 = vpaddq_u8(vandq_u8(match_op_2, bit_mask), vandq_u8(match_op_3, bit_mask));
  uint8x16_t ws_sum1 = vpaddq_u8(vandq_u8(match_ws_2, bit_mask), vandq_u8(match_ws_3, bit_mask));
  op_sum0 = vpaddq_u8(op_sum0, op_sum1);
  ws_sum0 = vpaddq_u8(ws_sum0, ws_sum1);
  op_sum0 = vpaddq_u8(op_sum0, op_sum0);
  ws_sum0 = vpaddq_u8(ws_sum0, ws_sum0);
  const uint64_t op = vgetq_lane_u64(vreinterpretq_u64_u8(op_sum0), 0);
  const uint64_t whitespace = vgetq_lane_u64(vreinterpretq_u64_u8(ws_sum0), 0);

  return { whitespace, op };
}

simdjson_inline bool is_ascii(const simd8x64<uint8_t>& input) {
    simd8<uint8_t> bits = input.reduce_or();
    return bits.max_val() < 0x80u;
}

simdjson_unused simdjson_inline simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
    simd8<bool> is_second_byte = prev1 >= uint8_t(0xc0u);
    simd8<bool> is_third_byte  = prev2 >= uint8_t(0xe0u);
    simd8<bool> is_fourth_byte = prev3 >= uint8_t(0xf0u);
    // Use ^ instead of | for is_*_byte, because ^ is commutative, and the caller is using ^ as well.
    // This will work fine because we only have to report errors for cases with 0-1 lead bytes.
    // Multiple lead bytes implies 2 overlapping multibyte characters, and if that happens, there is
    // guaranteed to be at least *one* lead byte that is part of only 1 other multibyte character.
    // The error will be detected there.
    return is_second_byte ^ is_third_byte ^ is_fourth_byte;
}

simdjson_inline simd8<uint8_t> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
    simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0xe0u-0x80); // Only 111_____ will be >= 0x80
    simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0xf0u-0x80); // Only 1111____ will be >= 0x80
    return is_third_byte | is_fourth_byte;
}

} // unnamed namespace
} // namespace arm64
} // namespace simdjson

//
// Stage 2
//

//
// Implementation-specific overrides
//
namespace simdjson {
namespace arm64 {

simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return arm64::stage1::json_minifier::minify<64>(buf, len, dst, dst_len);
}

simdjson_flatten simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return arm64::stage1::json_structural_indexer::index<64>(buf, len, *this, streaming);
}

simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return arm64::stage1::generic_validate_utf8(buf,len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_NO_SANITIZE_MEMORY
simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst, bool allow_replacement) const noexcept {
  return arm64::stringparsing::parse_string(src, dst, allow_replacement);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return arm64::stringparsing::parse_wobbly_string(src, dst);
}

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace arm64
} // namespace simdjson

#include <simdjson/arm64/end.h>

#endif // SIMDJSON_SRC_ARM64_CPP
