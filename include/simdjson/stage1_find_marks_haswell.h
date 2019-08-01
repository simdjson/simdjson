#ifndef SIMDJSON_STAGE1_FIND_MARKS_HASWELL_H
#define SIMDJSON_STAGE1_FIND_MARKS_HASWELL_H

#include "simdjson/simdutf8check_haswell.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage1_find_marks_flatten_haswell.h"
#include "simdjson/stage1_find_marks_macros.h"

#ifdef IS_X86_64

TARGET_HASWELL
namespace simdjson {
template <> struct simd_input<Architecture::HASWELL> {
  __m256i lo;
  __m256i hi;
};

template <>
really_inline simd_input<Architecture::HASWELL>
fill_input<Architecture::HASWELL>(const uint8_t *ptr) {
  struct simd_input<Architecture::HASWELL> in;
  in.lo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 0));
  in.hi = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 32));
  return in;
}

template <>
really_inline uint64_t
compute_quote_mask<Architecture::HASWELL>(uint64_t quote_bits) {
  // There should be no such thing with a processing supporting avx2
  // but not clmul.
  uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFFu), 0));
  return quote_mask;
}

template <> struct utf8_checking_state<Architecture::HASWELL> {
  __m256i has_error;
  avx_processed_utf_bytes previous;
  utf8_checking_state() {
    has_error = _mm256_setzero_si256();
    previous.raw_bytes = _mm256_setzero_si256();
    previous.high_nibbles = _mm256_setzero_si256();
    previous.carried_continuations = _mm256_setzero_si256();
  }
};

template <>
really_inline void check_utf8<Architecture::HASWELL>(
    simd_input<Architecture::HASWELL> in,
    utf8_checking_state<Architecture::HASWELL> &state) {
  __m256i high_bit = _mm256_set1_epi8(0x80u);
  if ((_mm256_testz_si256(_mm256_or_si256(in.lo, in.hi), high_bit)) == 1) {
    // it is ascii, we just check continuation
    state.has_error = _mm256_or_si256(
        _mm256_cmpgt_epi8(state.previous.carried_continuations,
                          _mm256_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                           9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                           9, 9, 9, 9, 9, 9, 9, 1)),
        state.has_error);
  } else {
    // it is not ascii so we have to do heavy work
    state.previous =
        avx_check_utf8_bytes(in.lo, &(state.previous), &(state.has_error));
    state.previous =
        avx_check_utf8_bytes(in.hi, &(state.previous), &(state.has_error));
  }
}

template <>
really_inline ErrorValues check_utf8_errors<Architecture::HASWELL>(
    utf8_checking_state<Architecture::HASWELL> &state) {
  return _mm256_testz_si256(state.has_error, state.has_error) == 0
             ? simdjson::UTF8_ERROR
             : simdjson::SUCCESS;
}

template <>
really_inline uint64_t cmp_mask_against_input<Architecture::HASWELL>(
    simd_input<Architecture::HASWELL> in, uint8_t m) {
  const __m256i mask = _mm256_set1_epi8(m);
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(in.lo, mask);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(in.hi, mask);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

template <>
really_inline uint64_t unsigned_lteq_against_input<Architecture::HASWELL>(
    simd_input<Architecture::HASWELL> in, uint8_t m) {
  const __m256i maxval = _mm256_set1_epi8(m);
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(_mm256_max_epu8(maxval, in.lo), maxval);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(_mm256_max_epu8(maxval, in.hi), maxval);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

template <>
really_inline uint64_t find_odd_backslash_sequences<Architecture::HASWELL>(
    simd_input<Architecture::HASWELL> in,
    uint64_t &prev_iter_ends_odd_backslash) {
  FIND_ODD_BACKSLASH_SEQUENCES(Architecture::HASWELL, in,
                               prev_iter_ends_odd_backslash);
}

template <>
really_inline uint64_t find_quote_mask_and_bits<Architecture::HASWELL>(
    simd_input<Architecture::HASWELL> in, uint64_t odd_ends,
    uint64_t &prev_iter_inside_quote, uint64_t &quote_bits,
    uint64_t &error_mask) {
  FIND_QUOTE_MASK_AND_BITS(Architecture::HASWELL, in, odd_ends,
                           prev_iter_inside_quote, quote_bits, error_mask)
}

template <>
really_inline void find_whitespace_and_structurals<Architecture::HASWELL>(
    simd_input<Architecture::HASWELL> in, uint64_t &whitespace,
    uint64_t &structurals) {
#ifdef SIMDJSON_NAIVE_STRUCTURAL
  // You should never need this naive approach, but it can be useful
  // for research purposes
  const __m256i mask_open_brace = _mm256_set1_epi8(0x7b);
  __m256i struct_lo = _mm256_cmpeq_epi8(in.lo, mask_open_brace);
  __m256i struct_hi = _mm256_cmpeq_epi8(in.hi, mask_open_brace);
  const __m256i mask_close_brace = _mm256_set1_epi8(0x7d);
  struct_lo =
      _mm256_or_si256(struct_lo, _mm256_cmpeq_epi8(in.lo, mask_close_brace));
  struct_hi =
      _mm256_or_si256(struct_hi, _mm256_cmpeq_epi8(in.hi, mask_close_brace));
  const __m256i mask_open_bracket = _mm256_set1_epi8(0x5b);
  struct_lo =
      _mm256_or_si256(struct_lo, _mm256_cmpeq_epi8(in.lo, mask_open_bracket));
  struct_hi =
      _mm256_or_si256(struct_hi, _mm256_cmpeq_epi8(in.hi, mask_open_bracket));
  const __m256i mask_close_bracket = _mm256_set1_epi8(0x5d);
  struct_lo =
      _mm256_or_si256(struct_lo, _mm256_cmpeq_epi8(in.lo, mask_close_bracket));
  struct_hi =
      _mm256_or_si256(struct_hi, _mm256_cmpeq_epi8(in.hi, mask_close_bracket));
  const __m256i mask_column = _mm256_set1_epi8(0x3a);
  struct_lo = _mm256_or_si256(struct_lo, _mm256_cmpeq_epi8(in.lo, mask_column));
  struct_hi = _mm256_or_si256(struct_hi, _mm256_cmpeq_epi8(in.hi, mask_column));
  const __m256i mask_comma = _mm256_set1_epi8(0x2c);
  struct_lo = _mm256_or_si256(struct_lo, _mm256_cmpeq_epi8(in.lo, mask_comma));
  struct_hi = _mm256_or_si256(struct_hi, _mm256_cmpeq_epi8(in.hi, mask_comma));
  uint64_t structural_res_0 =
      static_cast<uint32_t>(_mm256_movemask_epi8(struct_lo));
  uint64_t structural_res_1 = _mm256_movemask_epi8(struct_hi);
  structurals = (structural_res_0 | (structural_res_1 << 32));

  const __m256i mask_space = _mm256_set1_epi8(0x20);
  __m256i space_lo = _mm256_cmpeq_epi8(in.lo, mask_space);
  __m256i space_hi = _mm256_cmpeq_epi8(in.hi, mask_space);
  const __m256i mask_linefeed = _mm256_set1_epi8(0x0a);
  space_lo = _mm256_or_si256(space_lo, _mm256_cmpeq_epi8(in.lo, mask_linefeed));
  space_hi = _mm256_or_si256(space_hi, _mm256_cmpeq_epi8(in.hi, mask_linefeed));
  const __m256i mask_tab = _mm256_set1_epi8(0x09);
  space_lo = _mm256_or_si256(space_lo, _mm256_cmpeq_epi8(in.lo, mask_tab));
  space_hi = _mm256_or_si256(space_hi, _mm256_cmpeq_epi8(in.hi, mask_tab));
  const __m256i mask_carriage = _mm256_set1_epi8(0x0d);
  space_lo = _mm256_or_si256(space_lo, _mm256_cmpeq_epi8(in.lo, mask_carriage));
  space_hi = _mm256_or_si256(space_hi, _mm256_cmpeq_epi8(in.hi, mask_carriage));

  uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(space_lo));
  uint64_t ws_res_1 = _mm256_movemask_epi8(space_hi);
  whitespace = (ws_res_0 | (ws_res_1 << 32));
  // end of naive approach

#else  // SIMDJSON_NAIVE_STRUCTURAL
  // clang-format off
  const __m256i structural_table =
      _mm256_setr_epi8(44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123,
                       44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123);
  const __m256i white_table = _mm256_setr_epi8(
      32, 100, 100, 100, 17, 100, 113, 2, 100, 9, 10, 112, 100, 13, 100, 100,
      32, 100, 100, 100, 17, 100, 113, 2, 100, 9, 10, 112, 100, 13, 100, 100);
  // clang-format on
  const __m256i struct_offset = _mm256_set1_epi8(0xd4u);
  const __m256i struct_mask = _mm256_set1_epi8(32);

  __m256i lo_white =
      _mm256_cmpeq_epi8(in.lo, _mm256_shuffle_epi8(white_table, in.lo));
  __m256i hi_white =
      _mm256_cmpeq_epi8(in.hi, _mm256_shuffle_epi8(white_table, in.hi));
  uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(lo_white));
  uint64_t ws_res_1 = _mm256_movemask_epi8(hi_white);
  whitespace = (ws_res_0 | (ws_res_1 << 32));
  __m256i lo_struct_r1 = _mm256_add_epi8(struct_offset, in.lo);
  __m256i hi_struct_r1 = _mm256_add_epi8(struct_offset, in.hi);
  __m256i lo_struct_r2 = _mm256_or_si256(in.lo, struct_mask);
  __m256i hi_struct_r2 = _mm256_or_si256(in.hi, struct_mask);
  __m256i lo_struct_r3 = _mm256_shuffle_epi8(structural_table, lo_struct_r1);
  __m256i hi_struct_r3 = _mm256_shuffle_epi8(structural_table, hi_struct_r1);
  __m256i lo_struct = _mm256_cmpeq_epi8(lo_struct_r2, lo_struct_r3);
  __m256i hi_struct = _mm256_cmpeq_epi8(hi_struct_r2, hi_struct_r3);

  uint64_t structural_res_0 =
      static_cast<uint32_t>(_mm256_movemask_epi8(lo_struct));
  uint64_t structural_res_1 = _mm256_movemask_epi8(hi_struct);
  structurals = (structural_res_0 | (structural_res_1 << 32));
#endif // SIMDJSON_NAIVE_STRUCTURAL
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_STAGE1_FIND_MARKS_HASWELL_H