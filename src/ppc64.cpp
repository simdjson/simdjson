#ifndef SIMDJSON_SRC_PPC64_CPP
#define SIMDJSON_SRC_PPC64_CPP

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include <base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <simdjson/ppc64.h>
#include <simdjson/ppc64/implementation.h>

#include <simdjson/ppc64/begin.h>
#include <generic/amalgamated.h>
#include <generic/stage1/amalgamated.h>
#include <generic/stage2/amalgamated.h>

//
// Stage 1
//
namespace simdjson {
namespace ppc64 {

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

enum op_whitespace_t : uint8_t {
  OPEN_OR_CLOSE = 1u << 0,
  COLON         = 1u << 1,
  COMMA         = 1u << 2,
  TAB_CR_LF     = 1u << 3,
  SPACE         = 1u << 4,
};

simdjson_constinit byte_classifier OP_WHITESPACE_CLASSIFIER({
  _lookup_entry{ ' ',  op_whitespace_t::SPACE },
  { '\t', op_whitespace_t::TAB_CR_LF },
  { '\r', op_whitespace_t::TAB_CR_LF },
  { '\n', op_whitespace_t::TAB_CR_LF },
  { ':',  op_whitespace_t::COLON },
  { ',',  op_whitespace_t::COMMA },
  { '{',  op_whitespace_t::OPEN_OR_CLOSE },
  { '[',  op_whitespace_t::OPEN_OR_CLOSE },
  { '}',  op_whitespace_t::OPEN_OR_CLOSE },
  { ']',  op_whitespace_t::OPEN_OR_CLOSE },
});

simdjson_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t>& in) {
  // Functional programming causes trouble with Visual Studio.
  // Keeping this version in comments since it is much nicer:
  // auto v = in.map<uint8_t>([&](simd8<uint8_t> chunk) {
  //  auto nib_lo = chunk & 0xf;
  //  auto nib_hi = chunk.shr<4>();
  //  auto shuf_lo = nib_lo.lookup_16<uint8_t>(16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
  //  auto shuf_hi = nib_hi.lookup_16<uint8_t>(8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0);
  //  return shuf_lo & shuf_hi;
  // });
  simd8x64<uint8_t> op_whitespace = OP_WHITESPACE_CLASSIFIER[in];

  // We compute whitespace and op separately. If the code later only use one or the
  // other, given the fact that all functions are aggressively inlined, we can
  // hope that useless computations will be omitted. This is namely case when
  // minifying (we only need whitespace). *However* if we only need spaces,
  // it is likely that we will still compute 'v' above with two lookup_16: one
  // could do it a bit cheaper. This is in contrast with the x64 implementations
  // where we can, efficiently, do the white space and structural matching
  // separately. One reason for this difference is that on ARM NEON, the table
  // lookups either zero or leave unchanged the characters exceeding 0xF whereas
  // on x64, the equivalent instruction (pshufb) automatically applies a mask,
  // ignoring the 4 most significant bits. Thus the x64 implementation is
  // optimized differently. This being said, if you use this code strictly
  // just for minification (or just to identify the structural characters),
  // there is a small untaken optimization opportunity here. We deliberately
  // do not pick it up.

  uint64_t op = op_whitespace.any_bits_set(
    op_whitespace_t::SPACE | op_whitespace_t::TAB_CR_LF
  ).to_bitmask();

  uint64_t whitespace = op_whitespace.any_bits_set(
    op_whitespace_t::COLON | op_whitespace_t::COMMA | op_whitespace_t::OPEN_OR_CLOSE
  ).to_bitmask();

  return { whitespace, op };
}

simdjson_inline bool is_ascii(const simd8x64<uint8_t>& input) {
  // careful: 0x80 is not ascii.
  return input.reduce_or().saturating_sub(0x7fu).bits_not_set_anywhere();
}

simdjson_unused simdjson_inline simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0xc0u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0xe0u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0xf0u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
}

simdjson_inline simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0xe0u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0xf0u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_third_byte | is_fourth_byte) > int8_t(0);
}

} // unnamed namespace
} // namespace ppc64
} // namespace simdjson

//
// Stage 2
//

//
// Implementation-specific overrides
//
namespace simdjson {
namespace ppc64 {

simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return ppc64::stage1::json_minifier::minify<64>(buf, len, dst, dst_len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return ppc64::stage1::json_structural_indexer::index<64>(buf, len, *this, streaming);
}

simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return ppc64::stage1::generic_validate_utf8(buf,len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst, bool replacement_char) const noexcept {
  return ppc64::stringparsing::parse_string(src, dst, replacement_char);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return ppc64::stringparsing::parse_wobbly_string(src, dst);
}

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace ppc64
} // namespace simdjson

#include <simdjson/ppc64/end.h>

#endif // SIMDJSON_SRC_PPC64_CPP