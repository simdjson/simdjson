#include "simdjson.h"
#include "westmere/implementation.h"
#include "westmere/dom_parser_implementation.h"

//
// Stage 1
//
#include "westmere/bitmask.h"
#include "westmere/simd.h"
#include "westmere/bitmanipulation.h"
#include "westmere/implementation.h"

TARGET_WESTMERE
namespace simdjson {
namespace westmere {

using namespace simd;

struct json_character_block {
  static really_inline json_character_block classify(const simd::simd8x64<uint8_t> in);

  really_inline uint64_t whitespace() const { return _whitespace; }
  really_inline uint64_t op() const { return _op; }
  really_inline uint64_t scalar() { return ~(op() | whitespace()); }

  uint64_t _whitespace;
  uint64_t _op;
};

really_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t> in) {
  // These lookups rely on the fact that anything < 127 will match the lower 4 bits, which is why
  // we can't use the generic lookup_16.
  auto whitespace_table = simd8<uint8_t>::repeat_16(' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112, 100, '\r', 100, 100);
  auto op_table = simd8<uint8_t>::repeat_16(',', '}', 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '{');

  // We compute whitespace and op separately. If the code later only use one or the
  // other, given the fact that all functions are aggressively inlined, we can
  // hope that useless computations will be omitted. This is namely case when
  // minifying (we only need whitespace).

  uint64_t whitespace = in.map([&](simd8<uint8_t> _in) {
    return _in == simd8<uint8_t>(_mm_shuffle_epi8(whitespace_table, _in));
  }).to_bitmask();

  uint64_t op = in.map([&](simd8<uint8_t> _in) {
    // | 32 handles the fact that { } and [ ] are exactly 32 bytes apart
    return (_in | 32) == simd8<uint8_t>(_mm_shuffle_epi8(op_table, _in-','));
  }).to_bitmask();
  return { whitespace, op };
}

really_inline bool is_ascii(simd8x64<uint8_t> input) {
  simd8<uint8_t> bits = input.reduce([&](simd8<uint8_t> a,simd8<uint8_t> b) { return a|b; });
  return !bits.any_bits_set_anywhere(0b10000000u);
}

really_inline simd8<bool> must_be_continuation(simd8<uint8_t> prev1, simd8<uint8_t> prev2, simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0b11000000u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
}

really_inline simd8<bool> must_be_2_3_continuation(simd8<uint8_t> prev2, simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_third_byte | is_fourth_byte) > int8_t(0);
}


#include "generic/stage1/buf_block_reader.h"
#include "generic/stage1/json_string_scanner.h"
#include "generic/stage1/json_scanner.h"

namespace stage1 {
really_inline uint64_t json_string_scanner::find_escaped(uint64_t backslash) {
  if (!backslash) { uint64_t escaped = prev_escaped; prev_escaped = 0; return escaped; }
  return find_escaped_branchless(backslash);
}
}

#include "generic/stage1/json_minifier.h"
WARN_UNUSED error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return westmere::stage1::json_minifier::minify<64>(buf, len, dst, dst_len);
}

#include "generic/stage1/find_next_document_index.h"
#include "generic/stage1/utf8_lookup3_algorithm.h"
#include "generic/stage1/json_structural_indexer.h"
WARN_UNUSED error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, bool streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return westmere::stage1::json_structural_indexer::index<64>(_buf, _len, *this, streaming);
}
#include "generic/stage1/utf8_validator.h"
WARN_UNUSED bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return simdjson::westmere::stage1::generic_validate_utf8(buf,len);
}
} // namespace westmere
} // namespace simdjson
UNTARGET_REGION

//
// Stage 2
//
#include "westmere/stringparsing.h"
#include "westmere/numberparsing.h"

TARGET_WESTMERE
namespace simdjson {
namespace westmere {

#include "generic/stage2/logger.h"
#include "generic/stage2/atomparsing.h"
#include "generic/stage2/structural_iterator.h"
#include "generic/stage2/structural_parser.h"

WARN_UNUSED error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  error_code err = stage1(_buf, _len, false);
  if (err) { return err; }
  return stage2(_doc);
}

} // namespace westmere
} // namespace simdjson
UNTARGET_REGION
