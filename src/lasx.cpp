#ifndef SIMDJSON_SRC_LASX_CPP
#define SIMDJSON_SRC_LASX_CPP

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include <base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <simdjson/lasx.h>
#include <simdjson/lasx/implementation.h>

#include <simdjson/lasx/begin.h>
#include <generic/amalgamated.h>
#include <generic/stage1/amalgamated.h>
#include <generic/stage2/amalgamated.h>

//
// Stage 1
//
namespace simdjson {
namespace lasx {

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
  // Inspired by haswell.
  // LASX use low 5 bits as index. For the 6 operators (:,[]{}), the unique-5bits is [6:2].
  // The ASCII white-space and operators have these values: (char, hex, unique-5bits)
  // (' ', 20, 00000) ('\t', 09, 01001) ('\n', 0A, 01010) ('\r', 0D, 01101)
  // (',', 2C, 01011) (':', 3A, 01110) ('[', 5B, 10110) ('{', 7B, 11110) (']', 5D, 10111) ('}', 7D, 11111)
  const simd8<uint8_t> ws_table = simd8<uint8_t>::repeat_16(
    ' ', 0, 0, 0, 0, 0, 0, 0, 0, '\t', '\n', 0, 0, '\r', 0, 0
  );
  const simd8<uint8_t> op_table_lo = simd8<uint8_t>::repeat_16(
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ',', 0, 0, ':', 0
  );
  const simd8<uint8_t> op_table_hi = simd8<uint8_t>::repeat_16(
    0, 0, 0, 0, 0, 0, '[', ']', 0, 0, 0, 0, 0, 0, '{', '}'
  );
  uint64_t ws = in.eq({
    in.chunks[0].lookup_16(ws_table),
    in.chunks[1].lookup_16(ws_table),
  });
  uint64_t op = in.eq({
    __lasx_xvshuf_b(op_table_hi, op_table_lo, in.chunks[0].shr<2>()),
    __lasx_xvshuf_b(op_table_hi, op_table_lo, in.chunks[1].shr<2>()),
  });

  return { ws, op };
}

simdjson_inline bool is_ascii(const simd8x64<uint8_t>& input) {
  return input.reduce_or().is_ascii();
}

simdjson_inline simd8<uint8_t> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
    simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0xe0u-0x80); // Only 111_____ will be >= 0x80
    simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0xf0u-0x80); // Only 1111____ will be >= 0x80
    return is_third_byte | is_fourth_byte;
}

} // unnamed namespace
} // namespace lasx
} // namespace simdjson

//
// Stage 2
//

//
// Implementation-specific overrides
//
namespace simdjson {
namespace lasx {

simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return lasx::stage1::json_minifier::minify<64>(buf, len, dst, dst_len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return lasx::stage1::json_structural_indexer::index<64>(buf, len, *this, streaming);
}

simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return lasx::stage1::generic_validate_utf8(buf,len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst, bool allow_replacement) const noexcept {
  return lasx::stringparsing::parse_string(src, dst, allow_replacement);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return lasx::stringparsing::parse_wobbly_string(src, dst);
}

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace lasx
} // namespace simdjson

#include <simdjson/lasx/end.h>

#endif // SIMDJSON_SRC_LASX_CPP
