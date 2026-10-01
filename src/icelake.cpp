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

// UTF-8 rules only look at the three preceding bytes. Keep the vector work for
// each block independent, then reduce the three-byte boundary summaries.
struct utf8_block_summary {
  uint32_t suffix;
  bool is_ascii;
};

constexpr size_t UTF8_BATCH_MIN_LENGTH = 64 * 1024;

simdjson_inline uint32_t utf8_three_bytes(const uint8_t *input) {
  return uint32_t(input[0]) | (uint32_t(input[1]) << 8) | (uint32_t(input[2]) << 16);
}

simdjson_inline simd8<uint8_t> utf8_errors(const simd8<uint8_t>& input, const simd8<uint8_t>& prev_input) {
  const simd8<uint8_t> prev1 = input.prev<1>(prev_input);
  const simd8<uint8_t> special_cases = utf8_validation::check_special_cases(input, prev1);
  return utf8_validation::check_multibyte_lengths(input, prev_input, special_cases);
}

simdjson_inline simd8<uint8_t> utf8_previous_suffix(uint32_t suffix) {
  const __m128i suffix_bytes = _mm_slli_si128(_mm_cvtsi32_si128(static_cast<int>(suffix)), 13);
  return simd8<uint8_t>(_mm512_inserti32x4(_mm512_setzero_si512(), suffix_bytes, 3));
}

simdjson_never_inline bool validate_utf8_batched(const char *buf, size_t len) {
  // The public entry point calls this only for long inputs.
  SIMDJSON_ASSUME(len >= UTF8_BATCH_MIN_LENGTH);
  stage1::buf_block_reader<64> reader(reinterpret_cast<const uint8_t *>(buf), len);
  utf8_block_summary previous{ 0, true };
  bool has_error = false;
  const size_t full_block_count = (len - 1) / 64;
  size_t full_blocks_done = 0;

  // Load eight blocks before rebuilding their first predecessor lanes from a
  // compact suffix summary. The remaining blocks use independently loaded
  // neighbors, never a preceding validation result.
  while (full_block_count - full_blocks_done >= 8) {
    const uint8_t *input0 = reader.full_block(); reader.advance();
    const uint8_t *input1 = reader.full_block(); reader.advance();
    const uint8_t *input2 = reader.full_block(); reader.advance();
    const uint8_t *input3 = reader.full_block(); reader.advance();
    const uint8_t *input4 = reader.full_block(); reader.advance();
    const uint8_t *input5 = reader.full_block(); reader.advance();
    const uint8_t *input6 = reader.full_block(); reader.advance();
    const uint8_t *input7 = reader.full_block(); reader.advance();
    full_blocks_done += 8;

    const simd8<uint8_t> block0(input0);
    const simd8<uint8_t> block1(input1);
    const simd8<uint8_t> block2(input2);
    const simd8<uint8_t> block3(input3);
    const simd8<uint8_t> block4(input4);
    const simd8<uint8_t> block5(input5);
    const simd8<uint8_t> block6(input6);
    const simd8<uint8_t> block7(input7);
    const bool ascii0 = block0.is_ascii();
    const bool ascii1 = block1.is_ascii();
    const bool ascii2 = block2.is_ascii();
    const bool ascii3 = block3.is_ascii();
    const bool ascii4 = block4.is_ascii();
    const bool ascii5 = block5.is_ascii();
    const bool ascii6 = block6.is_ascii();
    const bool ascii7 = block7.is_ascii();
    if (ascii0 && ascii1 && ascii2 && ascii3 && ascii4 && ascii5 && ascii6 && ascii7 && previous.is_ascii) {
      previous = { utf8_three_bytes(input7 + 61), true };
    } else {
      const simd8<uint8_t> error0 = (!ascii0 || !previous.is_ascii) ?
          utf8_errors(block0, utf8_previous_suffix(previous.suffix)) : simd8<uint8_t>();
      const simd8<uint8_t> error1 = (!ascii1 || !ascii0) ? utf8_errors(block1, block0) : simd8<uint8_t>();
      const simd8<uint8_t> error2 = (!ascii2 || !ascii1) ? utf8_errors(block2, block1) : simd8<uint8_t>();
      const simd8<uint8_t> error3 = (!ascii3 || !ascii2) ? utf8_errors(block3, block2) : simd8<uint8_t>();
      const simd8<uint8_t> error4 = (!ascii4 || !ascii3) ? utf8_errors(block4, block3) : simd8<uint8_t>();
      const simd8<uint8_t> error5 = (!ascii5 || !ascii4) ? utf8_errors(block5, block4) : simd8<uint8_t>();
      const simd8<uint8_t> error6 = (!ascii6 || !ascii5) ? utf8_errors(block6, block5) : simd8<uint8_t>();
      const simd8<uint8_t> error7 = (!ascii7 || !ascii6) ? utf8_errors(block7, block6) : simd8<uint8_t>();
      has_error |= (error0 | error1 | error2 | error3 | error4 | error5 | error6 | error7).any_bits_set_anywhere();
      previous = { utf8_three_bytes(input7 + 61), ascii7 };
    }
  }

  // Load four blocks before rebuilding their predecessor lanes from compact
  // suffix summaries. The expensive vector checks share no result dependency.
  while (full_block_count - full_blocks_done >= 4) {
    const uint8_t *input0 = reader.full_block(); reader.advance();
    const uint8_t *input1 = reader.full_block(); reader.advance();
    const uint8_t *input2 = reader.full_block(); reader.advance();
    const uint8_t *input3 = reader.full_block(); reader.advance();
    full_blocks_done += 4;

    const simd8<uint8_t> block0(input0);
    const simd8<uint8_t> block1(input1);
    const simd8<uint8_t> block2(input2);
    const simd8<uint8_t> block3(input3);
    const bool ascii0 = block0.is_ascii();
    const bool ascii1 = block1.is_ascii();
    const bool ascii2 = block2.is_ascii();
    const bool ascii3 = block3.is_ascii();
    if (ascii0 && ascii1 && ascii2 && ascii3 && previous.is_ascii) {
      previous = { utf8_three_bytes(input3 + 61), true };
    } else {
      const simd8<uint8_t> error0 = (!ascii0 || !previous.is_ascii) ?
          utf8_errors(block0, utf8_previous_suffix(previous.suffix)) : simd8<uint8_t>();
      const simd8<uint8_t> error1 = (!ascii1 || !ascii0) ? utf8_errors(block1, block0) : simd8<uint8_t>();
      const simd8<uint8_t> error2 = (!ascii2 || !ascii1) ? utf8_errors(block2, block1) : simd8<uint8_t>();
      const simd8<uint8_t> error3 = (!ascii3 || !ascii2) ? utf8_errors(block3, block2) : simd8<uint8_t>();
      has_error |= (error0 | error1 | error2 | error3).any_bits_set_anywhere();
      previous = { utf8_three_bytes(input3 + 61), ascii3 };
    }
  }

  while (full_blocks_done < full_block_count) {
    const uint8_t *input = reader.full_block();
    reader.advance();
    ++full_blocks_done;
    const simd8<uint8_t> block(input);
    const bool block_is_ascii = block.is_ascii();
    if (!block_is_ascii || !previous.is_ascii) {
      has_error |= utf8_errors(block, utf8_previous_suffix(previous.suffix)).any_bits_set_anywhere();
    }
    previous = { utf8_three_bytes(input + 61), block_is_ascii };
  }

  uint8_t remainder[64]{};
  reader.get_remainder(remainder);
  const simd8<uint8_t> last_block(remainder);
  const bool last_is_ascii = last_block.is_ascii();
  if (!last_is_ascii || !previous.is_ascii) {
    has_error |= utf8_errors(last_block, utf8_previous_suffix(previous.suffix)).any_bits_set_anywhere();
  }
  if (!last_is_ascii) {
    has_error |= utf8_validation::is_incomplete(last_block).any_bits_set_anywhere();
  }
  return !has_error;
}

simdjson_unused simdjson_inline simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0xc0u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0xe0u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0xf0u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
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
  return stage2::tape_builder::parse_document<true>(*this, _doc);
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
