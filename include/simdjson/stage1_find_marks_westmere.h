#ifndef SIMDJSON_STAGE1_FIND_MARKS_WESTMERE_H
#define SIMDJSON_STAGE1_FIND_MARKS_WESTMERE_H

#include "simdjson/simdutf8check_westmere.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage1_find_marks_flatten.h"
#include "simdjson/stage1_find_marks_macros.h"

#ifdef IS_X86_64

TARGET_WESTMERE
namespace simdjson {
template <> struct simd_input<Architecture::WESTMERE> {
  __m128i v0;
  __m128i v1;
  __m128i v2;
  __m128i v3;
};

template <>
really_inline simd_input<Architecture::WESTMERE>
fill_input<Architecture::WESTMERE>(const uint8_t *ptr) {
  struct simd_input<Architecture::WESTMERE> in;
  in.v0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 0));
  in.v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 16));
  in.v2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 32));
  in.v3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 48));
  return in;
}

template <>
really_inline uint64_t
compute_quote_mask<Architecture::WESTMERE>(uint64_t quote_bits) {
  return _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFFu), 0));
}

template <> struct utf8_checking_state<Architecture::WESTMERE> {
  __m128i has_error = _mm_setzero_si128();
  processed_utf_bytes previous{
      _mm_setzero_si128(), // raw_bytes
      _mm_setzero_si128(), // high_nibbles
      _mm_setzero_si128()  // carried_continuations
  };
};

template <>
really_inline void check_utf8<Architecture::WESTMERE>(
    simd_input<Architecture::WESTMERE> in,
    utf8_checking_state<Architecture::WESTMERE> &state) {
  __m128i high_bit = _mm_set1_epi8(0x80u);
  if ((_mm_testz_si128(_mm_or_si128(in.v0, in.v1), high_bit)) == 1) {
    // it is ascii, we just check continuation
    state.has_error =
        _mm_or_si128(_mm_cmpgt_epi8(state.previous.carried_continuations,
                                    _mm_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                                  9, 9, 9, 9, 9, 1)),
                     state.has_error);
  } else {
    // it is not ascii so we have to do heavy work
    state.previous =
        check_utf8_bytes(in.v0, &(state.previous), &(state.has_error));
    state.previous =
        check_utf8_bytes(in.v1, &(state.previous), &(state.has_error));
  }

  if ((_mm_testz_si128(_mm_or_si128(in.v2, in.v3), high_bit)) == 1) {
    // it is ascii, we just check continuation
    state.has_error =
        _mm_or_si128(_mm_cmpgt_epi8(state.previous.carried_continuations,
                                    _mm_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                                  9, 9, 9, 9, 9, 1)),
                     state.has_error);
  } else {
    // it is not ascii so we have to do heavy work
    state.previous =
        check_utf8_bytes(in.v2, &(state.previous), &(state.has_error));
    state.previous =
        check_utf8_bytes(in.v3, &(state.previous), &(state.has_error));
  }
}

template <>
really_inline ErrorValues check_utf8_errors<Architecture::WESTMERE>(
    utf8_checking_state<Architecture::WESTMERE> &state) {
  return _mm_testz_si128(state.has_error, state.has_error) == 0
             ? simdjson::UTF8_ERROR
             : simdjson::SUCCESS;
}

template <>
really_inline uint64_t cmp_mask_against_input<Architecture::WESTMERE>(
    simd_input<Architecture::WESTMERE> in, uint8_t m) {
  const __m128i mask = _mm_set1_epi8(m);
  __m128i cmp_res_0 = _mm_cmpeq_epi8(in.v0, mask);
  uint64_t res_0 = _mm_movemask_epi8(cmp_res_0);
  __m128i cmp_res_1 = _mm_cmpeq_epi8(in.v1, mask);
  uint64_t res_1 = _mm_movemask_epi8(cmp_res_1);
  __m128i cmp_res_2 = _mm_cmpeq_epi8(in.v2, mask);
  uint64_t res_2 = _mm_movemask_epi8(cmp_res_2);
  __m128i cmp_res_3 = _mm_cmpeq_epi8(in.v3, mask);
  uint64_t res_3 = _mm_movemask_epi8(cmp_res_3);
  return res_0 | (res_1 << 16) | (res_2 << 32) | (res_3 << 48);
}

template <>
really_inline uint64_t unsigned_lteq_against_input<Architecture::WESTMERE>(
    simd_input<Architecture::WESTMERE> in, uint8_t m) {
  const __m128i maxval = _mm_set1_epi8(m);
  __m128i cmp_res_0 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, in.v0), maxval);
  uint64_t res_0 = _mm_movemask_epi8(cmp_res_0);
  __m128i cmp_res_1 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, in.v1), maxval);
  uint64_t res_1 = _mm_movemask_epi8(cmp_res_1);
  __m128i cmp_res_2 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, in.v2), maxval);
  uint64_t res_2 = _mm_movemask_epi8(cmp_res_2);
  __m128i cmp_res_3 = _mm_cmpeq_epi8(_mm_max_epu8(maxval, in.v3), maxval);
  uint64_t res_3 = _mm_movemask_epi8(cmp_res_3);
  return res_0 | (res_1 << 16) | (res_2 << 32) | (res_3 << 48);
}

template <>
really_inline uint64_t find_odd_backslash_sequences<Architecture::WESTMERE>(
    simd_input<Architecture::WESTMERE> in,
    uint64_t &prev_iter_ends_odd_backslash) {
  FIND_ODD_BACKSLASH_SEQUENCES(Architecture::WESTMERE, in,
                               prev_iter_ends_odd_backslash);
}

template <>
really_inline uint64_t find_quote_mask_and_bits<Architecture::WESTMERE>(
    simd_input<Architecture::WESTMERE> in, uint64_t odd_ends,
    uint64_t &prev_iter_inside_quote, uint64_t &quote_bits,
    uint64_t &error_mask) {
  FIND_QUOTE_MASK_AND_BITS(Architecture::WESTMERE, in, odd_ends,
                           prev_iter_inside_quote, quote_bits, error_mask)
}

template <>
really_inline void find_whitespace_and_structurals<Architecture::WESTMERE>(
    simd_input<Architecture::WESTMERE> in, uint64_t &whitespace,
    uint64_t &structurals) {
  const __m128i structural_table =
      _mm_setr_epi8(44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123);
  const __m128i white_table = _mm_setr_epi8(32, 100, 100, 100, 17, 100, 113, 2,
                                            100, 9, 10, 112, 100, 13, 100, 100);
  const __m128i struct_offset = _mm_set1_epi8(0xd4u);
  const __m128i struct_mask = _mm_set1_epi8(32);

  __m128i white0 = _mm_cmpeq_epi8(in.v0, _mm_shuffle_epi8(white_table, in.v0));
  __m128i white1 = _mm_cmpeq_epi8(in.v1, _mm_shuffle_epi8(white_table, in.v1));
  __m128i white2 = _mm_cmpeq_epi8(in.v2, _mm_shuffle_epi8(white_table, in.v2));
  __m128i white3 = _mm_cmpeq_epi8(in.v3, _mm_shuffle_epi8(white_table, in.v3));
  uint64_t ws_res_0 = _mm_movemask_epi8(white0);
  uint64_t ws_res_1 = _mm_movemask_epi8(white1);
  uint64_t ws_res_2 = _mm_movemask_epi8(white2);
  uint64_t ws_res_3 = _mm_movemask_epi8(white3);

  whitespace =
      (ws_res_0 | (ws_res_1 << 16) | (ws_res_2 << 32) | (ws_res_3 << 48));

  __m128i struct1_r1 = _mm_add_epi8(struct_offset, in.v0);
  __m128i struct2_r1 = _mm_add_epi8(struct_offset, in.v1);
  __m128i struct3_r1 = _mm_add_epi8(struct_offset, in.v2);
  __m128i struct4_r1 = _mm_add_epi8(struct_offset, in.v3);

  __m128i struct1_r2 = _mm_or_si128(in.v0, struct_mask);
  __m128i struct2_r2 = _mm_or_si128(in.v1, struct_mask);
  __m128i struct3_r2 = _mm_or_si128(in.v2, struct_mask);
  __m128i struct4_r2 = _mm_or_si128(in.v3, struct_mask);

  __m128i struct1_r3 = _mm_shuffle_epi8(structural_table, struct1_r1);
  __m128i struct2_r3 = _mm_shuffle_epi8(structural_table, struct2_r1);
  __m128i struct3_r3 = _mm_shuffle_epi8(structural_table, struct3_r1);
  __m128i struct4_r3 = _mm_shuffle_epi8(structural_table, struct4_r1);

  __m128i struct1 = _mm_cmpeq_epi8(struct1_r2, struct1_r3);
  __m128i struct2 = _mm_cmpeq_epi8(struct2_r2, struct2_r3);
  __m128i struct3 = _mm_cmpeq_epi8(struct3_r2, struct3_r3);
  __m128i struct4 = _mm_cmpeq_epi8(struct4_r2, struct4_r3);

  uint64_t structural_res_0 = _mm_movemask_epi8(struct1);
  uint64_t structural_res_1 = _mm_movemask_epi8(struct2);
  uint64_t structural_res_2 = _mm_movemask_epi8(struct3);
  uint64_t structural_res_3 = _mm_movemask_epi8(struct4);

  structurals = (structural_res_0 | (structural_res_1 << 16) |
                 (structural_res_2 << 32) | (structural_res_3 << 48));
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_STAGE1_FIND_MARKS_WESTMERE_H