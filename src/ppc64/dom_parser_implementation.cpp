#include "simdjson/ppc64/begin.h"

//
// Stage 1
//
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {

using namespace simd;

struct json_character_block {
  static simdjson_really_inline json_character_block classify(const simd::simd8x64<uint8_t>& in);

  simdjson_really_inline uint64_t whitespace() const noexcept { return _whitespace; }
  simdjson_really_inline uint64_t op() const noexcept { return _op; }
  simdjson_really_inline uint64_t scalar() const noexcept { return ~(op() | whitespace()); }

  uint64_t _whitespace;
  uint64_t _op;
};

simdjson_really_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t>& in) {
  const simd8<uint8_t> table1(16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
  const simd8<uint8_t> table2(8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0);

  simd8x64<uint8_t> v(
     (in.chunks[0] & 0xf).lookup_16(table1) & (in.chunks[0].shr<4>()).lookup_16(table2),
     (in.chunks[1] & 0xf).lookup_16(table1) & (in.chunks[1].shr<4>()).lookup_16(table2),
     (in.chunks[2] & 0xf).lookup_16(table1) & (in.chunks[2].shr<4>()).lookup_16(table2),
     (in.chunks[3] & 0xf).lookup_16(table1) & (in.chunks[3].shr<4>()).lookup_16(table2)
  );

  uint64_t op = simd8x64<bool>(
        v.chunks[0].any_bits_set(0x7),
        v.chunks[1].any_bits_set(0x7),
        v.chunks[2].any_bits_set(0x7),
        v.chunks[3].any_bits_set(0x7)
  ).to_bitmask();

  uint64_t whitespace = simd8x64<bool>(
        v.chunks[0].any_bits_set(0x18),
        v.chunks[1].any_bits_set(0x18),
        v.chunks[2].any_bits_set(0x18),
        v.chunks[3].any_bits_set(0x18)
  ).to_bitmask();

  return { whitespace, op };
}

simdjson_really_inline bool is_ascii(const simd8x64<uint8_t>& input) {
  // careful: 0x80 is not ascii.
  return input.reduce_or().saturating_sub(0b01111111u).bits_not_set_anywhere();
}

simdjson_unused simdjson_really_inline simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0b11000000u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
}

simdjson_really_inline simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_third_byte | is_fourth_byte) > int8_t(0);
}

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#include "generic/stage1/utf8_lookup4_algorithm.h"
#include "generic/stage1/json_structural_indexer.h"
#include "generic/stage1/utf8_validator.h"

//
// Stage 2
//

#include "generic/stage2/tape_builder.h"

//
// Implementation-specific overrides
//
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

simdjson_really_inline uint64_t json_string_scanner::find_escaped(uint64_t backslash) {
  // On PPC, we don't short-circuit this if there are no backslashes, because the branch gives us no
  // benefit and therefore makes things worse.
  // if (!backslash) { uint64_t escaped = prev_escaped; prev_escaped = 0; return escaped; }
  return find_escaped_branchless(backslash);
}

} // namespace stage1
} // unnamed namespace

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

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#include "simdjson/ppc64/end.h"
