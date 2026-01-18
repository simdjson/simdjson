#ifndef SIMDJSON_SRC_RVV_VLS_CPP
#define SIMDJSON_SRC_RVV_VLS_CPP

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include <base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <simdjson/rvv-vls.h>
#include <simdjson/rvv-vls/implementation.h>

#include <simdjson/rvv-vls/begin.h>
#include <simdjson/rvv-vls/simd.h>
#include <generic/amalgamated.h>
#include <generic/stage1/amalgamated.h>
#include <generic/stage2/amalgamated.h>

//
// Stage 1
//
namespace simdjson {
namespace rvv_vls {

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
  static const uint8_t wsTable[16] = { ' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112, 100, '\r', 100, 100 };
  static const uint8_t opTable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '{', ',', '}', 0, 0 };
  vuint8_t vws = __riscv_vle8_v_u8m1(wsTable, 16);
  vuint8_t vop = __riscv_vle8_v_u8m1(opTable, 16);
  vuint8x64_t lo = __riscv_vand(in, 15, 64);
  vuint8x64_t curl = __riscv_vor(in, 0x20, 64);

#if __riscv_v_fixed_vlen == 128
  vboolx64_t mws = __riscv_vmseq(simdutf_vrgather_u8m1x4(vws, lo), in, 64);
  vboolx64_t mop = __riscv_vmseq(simdutf_vrgather_u8m1x4(vop, lo), curl, 64);
#elif __riscv_v_fixed_vlen == 256
  vboolx64_t mws = __riscv_vmseq(simdutf_vrgather_u8m1x2(vws, lo), in, 64);
  vboolx64_t mop = __riscv_vmseq(simdutf_vrgather_u8m1x2(vop, lo), curl, 64);
#else
  vboolx64_t mws = __riscv_vmseq(__riscv_vrgather(vws, lo, 64), in, 64);
  vboolx64_t mop = __riscv_vmseq(__riscv_vrgather(vop, lo, 64), curl, 64);
#endif

  return {
      __riscv_vmv_x(__riscv_vreinterpret_u64m1(mws)),
      __riscv_vmv_x(__riscv_vreinterpret_u64m1(mop))
  };
}

simdjson_inline bool is_ascii(const simd8x64<uint8_t>& input) {
  return input.is_ascii();
}

simdjson_inline simd8<uint8_t> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
    simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0xe0u-0x80); // Only 111_____ will be >= 0x80
    simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0xf0u-0x80); // Only 1111____ will be >= 0x80
    return is_third_byte | is_fourth_byte;
}

} // unnamed namespace
} // namespace rvv_vls
} // namespace simdjson

//
// Stage 2
//

//
// Implementation-specific overrides
//
namespace simdjson {
namespace rvv_vls {

simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return rvv_vls::stage1::json_minifier::minify<64>(buf, len, dst, dst_len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return rvv_vls::stage1::json_structural_indexer::index<64>(buf, len, *this, streaming);
}

simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return rvv_vls::stage1::generic_validate_utf8(buf,len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_NO_SANITIZE_MEMORY
simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst, bool allow_replacement) const noexcept {
  return rvv_vls::stringparsing::parse_string(src, dst, allow_replacement);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return rvv_vls::stringparsing::parse_wobbly_string(src, dst);
}

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace rvv_vls
} // namespace simdjson

#include <simdjson/rvv-vls/end.h>

#endif // SIMDJSON_SRC_RVV_VLS_CPP
