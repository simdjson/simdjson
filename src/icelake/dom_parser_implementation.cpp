#include "simdjson/icelake/begin.h"

//
// Stage 1
//

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {

using namespace simd;

struct json_character_block {
  static simdjson_really_inline json_character_block classify(const simd::simd8x64<uint8_t>& in);
  //  ASCII white-space ('\r','\n','\t',' ')
  simdjson_really_inline uint64_t whitespace() const noexcept;
  // non-quote structural characters (comma, colon, braces, brackets)
  simdjson_really_inline uint64_t op() const noexcept;
  // neither a structural character nor a white-space, so letters, numbers and quotes
  simdjson_really_inline uint64_t scalar() const noexcept;

  uint64_t _whitespace; // ASCII white-space ('\r','\n','\t',' ')
  uint64_t _op; // structural characters (comma, colon, braces, brackets but not quotes)
};

simdjson_really_inline uint64_t json_character_block::whitespace() const noexcept { return _whitespace; }
simdjson_really_inline uint64_t json_character_block::op() const noexcept { return _op; }
simdjson_really_inline uint64_t json_character_block::scalar() const noexcept { return ~(op() | whitespace()); }

// This identifies structural characters (comma, colon, braces, brackets),
// and ASCII white-space ('\r','\n','\t',' ').
simdjson_really_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t>& in) {
  // These lookups rely on the fact that anything < 127 will match the lower 4 bits, which is why
  // we can't use the generic lookup_16.
  const auto whitespace_table = simd8<uint8_t>::repeat_16(' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112, 100, '\r', 100, 100);

  // The 6 operators (:,[]{}) have these values:
  //
  // , 2C
  // : 3A
  // [ 5B
  // { 7B
  // ] 5D
  // } 7D
  //
  // If you use | 0x20 to turn [ and ] into { and }, the lower 4 bits of each character is unique.
  // We exploit this, using a simd 4-bit lookup to tell us which character match against, and then
  // match it (against | 0x20).
  //
  // To prevent recognizing other characters, everything else gets compared with 0, which cannot
  // match due to the | 0x20.
  //
  // NOTE: Due to the | 0x20, this ALSO treats <FF> and <SUB> (control characters 0C and 1A) like ,
  // and :. This gets caught in stage 2, which checks the actual character to ensure the right
  // operators are in the right places.
  const auto op_table = simd8<uint8_t>::repeat_16(
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, ':', '{', // : = 3A, [ = 5B, { = 7B
    ',', '}', 0, 0  // , = 2C, ] = 5D, } = 7D
  );

  // We compute whitespace and op separately. If later code only uses one or the
  // other, given the fact that all functions are aggressively inlined, we can
  // hope that useless computations will be omitted. This is namely case when
  // minifying (we only need whitespace).

  const uint64_t whitespace = in.eq({
    _mm512_shuffle_epi8(whitespace_table, in.chunks[0])
  });
  // Turn [ and ] into { and }
  const simd8x64<uint8_t> curlified{
    in.chunks[0] | 0x20
  };
  const uint64_t op = curlified.eq({
    _mm512_shuffle_epi8(op_table, in.chunks[0])
  });

  return { whitespace, op };
}

simdjson_really_inline bool is_ascii(const simd8x64<uint8_t>& input) {
  return input.reduce_or().is_ascii();
}

simdjson_unused simdjson_really_inline simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0xc0u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0xe0u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0xf0u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
}

simdjson_really_inline simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0xe0u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0xf0u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_third_byte | is_fourth_byte) > int8_t(0);
}

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#include "generic/stage1/utf8_lookup4_algorithm.h"
// defining SIMDJSON_CUSTOM_BIT_INDEXER allows us to provide our own bit_indexer::write
#define SIMDJSON_CUSTOM_BIT_INDEXER
#include "generic/stage1/json_structural_indexer.h"
// We must not forget to undefine it now:
#undef SIMDJSON_CUSTOM_BIT_INDEXER

/**
 * We provide a custom version of bit_indexer::write using
 * naked intrinsics.
 * TODO: make this code more elegant.
 */
// Under GCC 12, the intrinsic _mm512_extracti32x4_epi32 may generate 'maybe uninitialized'.
// as a workaround, we disable warnings within the following function.
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
namespace simdjson { namespace SIMDJSON_IMPLEMENTATION { namespace { namespace stage1 {
simdjson_really_inline void bit_indexer::write(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0) { return; }

    const __m512i indexes = _mm512_maskz_compress_epi8(bits, _mm512_set_epi32(
      0x3f3e3d3c, 0x3b3a3938, 0x37363534, 0x33323130,
      0x2f2e2d2c, 0x2b2a2928, 0x27262524, 0x23222120,
      0x1f1e1d1c, 0x1b1a1918, 0x17161514, 0x13121110,
      0x0f0e0d0c, 0x0b0a0908, 0x07060504, 0x03020100
    ));
    const __m512i start_index = _mm512_set1_epi32(idx);

    const auto count = count_ones(bits);
    __m512i t0 = _mm512_cvtepu8_epi32(_mm512_castsi512_si128(indexes));
    _mm512_storeu_si512(this->tail, _mm512_add_epi32(t0, start_index));

    if(count > 16) {
      const __m512i t1 = _mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(indexes, 1));
      _mm512_storeu_si512(this->tail + 16, _mm512_add_epi32(t1, start_index));
      if(count > 32) {
        const __m512i t2 = _mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(indexes, 2));
        _mm512_storeu_si512(this->tail + 32, _mm512_add_epi32(t2, start_index));
        if(count > 48) {
          const __m512i t3 = _mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(indexes, 3));
          _mm512_storeu_si512(this->tail + 48, _mm512_add_epi32(t3, start_index));
        }
      }
    }
    this->tail += count;
}
}}}}
SIMDJSON_POP_DISABLE_WARNINGS

#include "generic/stage1/utf8_validator.h"

//
// Stage 2
//
#include "generic/stage2/stringparsing.h"
#include "generic/stage2/tape_builder.h"

//
// Implementation-specific overrides
//
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

simdjson_really_inline uint64_t json_string_scanner::find_escaped(uint64_t backslash) {
  if (!backslash) { uint64_t escaped = prev_escaped; prev_escaped = 0; return escaped; }
  return find_escaped_branchless(backslash);
}

} // namespace stage1
} // unnamed namespace

simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return icelake::stage1::json_minifier::minify<128>(buf, len, dst, dst_len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return icelake::stage1::json_structural_indexer::index<128>(_buf, _len, *this, streaming);
}

simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return icelake::stage1::generic_validate_utf8(buf,len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return icelake::stringparsing::parse_string(src, dst);
}

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#include "simdjson/icelake/end.h"
