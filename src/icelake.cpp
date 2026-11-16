#ifndef SIMDJSON_SRC_ICELAKE_CPP
#define SIMDJSON_SRC_ICELAKE_CPP

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include <base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <simdjson/icelake.h>
#include <simdjson/icelake/implementation.h>

// defining SIMDJSON_GENERIC_JSON_STRUCTURAL_INDEXER_CUSTOM_BIT_INDEXER allows us to provide our own bit_indexer::write
#define SIMDJSON_GENERIC_JSON_STRUCTURAL_INDEXER_CUSTOM_BIT_INDEXER

#include <simdjson/icelake/begin.h>
#include <generic/amalgamated.h>
#include <generic/stage1/amalgamated.h>
#include <generic/stage2/amalgamated.h>

#undef SIMDJSON_GENERIC_JSON_STRUCTURAL_INDEXER_CUSTOM_BIT_INDEXER

//
// Stage 1
//

namespace simdjson {
namespace icelake {

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

// This identifies structural characters (comma, colon, braces, brackets),
// and ASCII white-space ('\r','\n','\t',' ').
simdjson_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t>& in) {
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

simdjson_inline bool is_ascii(const simd8x64<uint8_t>& input) {
  return input.reduce_or().is_ascii();
}

constexpr size_t UTF8_BATCH_MIN_LENGTH = 64 * 1024;

simdjson_never_inline bool validate_utf8_batched(const char *buf, size_t len) {
  stage1::buf_block_reader<64> reader(reinterpret_cast<const uint8_t *>(buf), len);
  utf8_validation::utf8_checker checker{};
  size_t full_blocks = (len - 1) / 64;
  while (full_blocks >= 8) {
    const simd8<uint8_t> block0(reader.full_block());
    const simd8<uint8_t> block1(reader.full_block() + 64);
    const simd8<uint8_t> block2(reader.full_block() + 128);
    const simd8<uint8_t> block3(reader.full_block() + 192);
    const simd8<uint8_t> block4(reader.full_block() + 256);
    const simd8<uint8_t> block5(reader.full_block() + 320);
    const simd8<uint8_t> block6(reader.full_block() + 384);
    const simd8<uint8_t> block7(reader.full_block() + 448);
    const simd8<uint8_t> combined = block0 | block1 | block2 | block3 | block4 | block5 | block6 | block7;
    // Repeating the ASCII check only ORs the same pending-continuation error.
    if (combined.is_ascii()) {
      checker.check_next_input(simd8x64<uint8_t>(combined));
    } else if (!(block0 & block1 & block2 & block3 & block4 & block5 & block6 & block7).is_ascii()) {
      // A common high-bit lane proves that every block needs vector validation.
      simd8<uint8_t> previous = checker.prev_input_block;
      for (size_t i = 0; i < 8; ++i) {
        const simd8<uint8_t> block(reader.full_block() + i * 64);
        checker.check_utf8_bytes(block, previous);
        previous = block;
      }
      checker.prev_input_block = previous;
      checker.prev_incomplete = utf8_validation::is_incomplete(previous);
    } else {
      // Locate the first non-ASCII block with a binary search. OR the other
      // seven blocks together to prove that it is the only one needing work.
      const simd8<uint8_t> low = block0 | block1 | block2 | block3;
      const simd8<uint8_t> high = block4 | block5 | block6 | block7;
      size_t index = low.is_ascii() ? 4 : 0;
      simd8<uint8_t> others = index ? low : high;
      const uint8_t *p = reader.full_block() + index * 64;
      const simd8<uint8_t> pair = simd8<uint8_t>(p) | simd8<uint8_t>(p + 64);
      if (pair.is_ascii()) {
        index += 2;
        others |= pair;
      } else {
        others |= simd8<uint8_t>(p + 128) | simd8<uint8_t>(p + 192);
      }
      p = reader.full_block() + index * 64;
      const simd8<uint8_t> first(p);
      if (first.is_ascii()) {
        ++index;
        others |= first;
      } else {
        others |= simd8<uint8_t>(p + 64);
      }
      // Multiple non-ASCII blocks without a common high-bit lane use the
      // original reader for the rest of this request, keeping its checker state.
      if (!others.is_ascii()) { break; }
      // ASCII before or after the selected block cannot complete a pending
      // sequence. Like check_next_input, leave the last non-ASCII block intact.
      if (index != 0) { checker.error |= checker.prev_incomplete; }
      const simd8<uint8_t> block(reader.full_block() + index * 64);
      checker.check_utf8_bytes(block, checker.prev_input_block);
      checker.prev_input_block = block;
      checker.prev_incomplete = utf8_validation::is_incomplete(block);
      if (index != 7) { checker.error |= checker.prev_incomplete; }
    }
    for (size_t i = 0; i < 8; ++i) { reader.advance(); }
    full_blocks -= 8;
  }
  while (reader.has_full_block()) {
    checker.check_next_input(simd8x64<uint8_t>(reader.full_block()));
    reader.advance();
  }
  // Preserve the ordinary unpadded remainder and EOF checks.
  uint8_t remainder[64]{};
  reader.get_remainder(remainder);
  checker.check_next_input(simd8x64<uint8_t>(remainder));
  checker.check_eof();
  return checker.errors() == SUCCESS;
}

simdjson_inline simd8<uint8_t> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0xe0u-0x80); // Only 111_____ will be >= 0x80
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0xf0u-0x80); // Only 1111____ will be >= 0x80
  return is_third_byte | is_fourth_byte;
}

} // unnamed namespace
} // namespace icelake
} // namespace simdjson

/**
 * We provide a custom version of bit_indexer::write using
 * naked intrinsics.
 * TODO: make this code more elegant.
 */
// Under GCC 12, the intrinsic _mm512_extracti32x4_epi32 may generate 'maybe uninitialized'.
// as a workaround, we disable warnings within the following function.
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
namespace simdjson { namespace icelake { namespace { namespace stage1 {
simdjson_inline void bit_indexer::write(uint32_t idx, uint64_t bits) {
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

//
// Stage 2
//

//
// Implementation-specific overrides
//
namespace simdjson {
namespace icelake {

simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return icelake::stage1::json_minifier::minify<128>(buf, len, dst, dst_len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return icelake::stage1::json_structural_indexer::index<128>(_buf, _len, *this, streaming);
}

simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  // Keep short requests on the stateful path; the batch setup amortizes after 64 KiB.
  if (len >= UTF8_BATCH_MIN_LENGTH) {
    return validate_utf8_batched(buf, len);
  }
  return icelake::stage1::generic_validate_utf8(buf,len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_next_document(*this, _doc);
}

internal::dom_parser_implementation::stage2_next_function_t dom_parser_implementation::stage2_next_function(bool bounded) const noexcept {
  return bounded ? stage2::tape_builder::parse_next_bounded_document : stage2::tape_builder::parse_next_document;
}

SIMDJSON_NO_SANITIZE_MEMORY
simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst, bool replacement_char) const noexcept {
  return icelake::stringparsing::parse_string(src, dst, replacement_char);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return icelake::stringparsing::parse_wobbly_string(src, dst);
}

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace icelake
} // namespace simdjson

#include <simdjson/icelake/end.h>

#endif // SIMDJSON_SRC_ICELAKE_CPP
