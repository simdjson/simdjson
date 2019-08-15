#ifndef SIMDJSON_SIMDUTF8CHECK_WESTMERE_H
#define SIMDJSON_SIMDUTF8CHECK_WESTMERE_H

#include "simdjson/portability.h"
#include "simdjson/simdutf8check.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifdef IS_X86_64

TARGET_WESTMERE

namespace simdjson {

#define RET_ERR_IDX 0 /* Define 1 to return index of first error char */

struct processed_utf_bytes {
  __m128i input;
  __m128i first_len;
};

/* Cached tables */
/*
 * Map high nibble of "First Byte" to legal character length minus 1
 * 0x00 ~ 0xBF --> 0
 * 0xC0 ~ 0xDF --> 1
 * 0xE0 ~ 0xEF --> 2
 * 0xF0 ~ 0xFF --> 3
 */
const __m128i first_len_tbl =
    _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3);
/* Map "First Byte" to 8-th item of range table (0xC2 ~ 0xF4) */
const __m128i first_range_tbl =
    _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8);
/*
 * Range table, map range index to min and max values
 * Index 0    : 00 ~ 7F (First Byte, ascii)
 * Index 1,2,3: 80 ~ BF (Second, Third, Fourth Byte)
 * Index 4    : A0 ~ BF (Second Byte after E0)
 * Index 5    : 80 ~ 9F (Second Byte after ED)
 * Index 6    : 90 ~ BF (Second Byte after F0)
 * Index 7    : 80 ~ 8F (Second Byte after F4)
 * Index 8    : C2 ~ F4 (First Byte, non ascii)
 * Index 9~15 : illegal: i >= 127 && i <= -128
 */
const __m128i range_min_tbl =
    _mm_setr_epi8(0x00, 0x80, 0x80, 0x80, 0xA0, 0x80, 0x90, 0x80, 0xC2, 0x7F,
                  0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F);
const __m128i range_max_tbl =
    _mm_setr_epi8(0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x9F, 0xBF, 0x8F, 0xF4, 0x80,
                  0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
/*
 * Tables for fast handling of four special First Bytes(E0,ED,F0,F4), after
 * which the Second Byte are not 80~BF. It contains "range index adjustment".
 * +------------+---------------+------------------+----------------+
 * | First Byte | original range| range adjustment | adjusted range |
 * +------------+---------------+------------------+----------------+
 * | E0         | 2             | 2                | 4              |
 * +------------+---------------+------------------+----------------+
 * | ED         | 2             | 3                | 5              |
 * +------------+---------------+------------------+----------------+
 * | F0         | 3             | 3                | 6              |
 * +------------+---------------+------------------+----------------+
 * | F4         | 4             | 4                | 8              |
 * +------------+---------------+------------------+----------------+
 */
/* index1 -> E0, index14 -> ED */
const __m128i df_ee_tbl =
    _mm_setr_epi8(0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0);
/* index1 -> F0, index5 -> F4 */
const __m128i ef_fe_tbl =
    _mm_setr_epi8(0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

/* 5x faster than naive method */
/* Return 0 - success, -1 - error, >0 - first error char(if RET_ERR_IDX = 1) */
static inline struct processed_utf_bytes
check_utf8_bytes(__m128i input, struct processed_utf_bytes *previous,
                 __m128i *has_error) {
  /* high_nibbles = input >> 4 */
  const __m128i high_nibbles =
      _mm_and_si128(_mm_srli_epi16(input, 4), _mm_set1_epi8(0x0F));

  /* first_len = legal character length minus 1 */
  /* 0 for 00~7F, 1 for C0~DF, 2 for E0~EF, 3 for F0~FF */
  /* first_len = first_len_tbl[high_nibbles] */
  __m128i first_len = _mm_shuffle_epi8(first_len_tbl, high_nibbles);

  /* First Byte: set range index to 8 for bytes within 0xC0 ~ 0xFF */
  /* range = first_range_tbl[high_nibbles] */
  __m128i range = _mm_shuffle_epi8(first_range_tbl, high_nibbles);

  /* Second Byte: set range index to first_len */
  /* 0 for 00~7F, 1 for C0~DF, 2 for E0~EF, 3 for F0~FF */
  /* range |= (first_len, prev_first_len) << 1 byte */
  range =
      _mm_or_si128(range, _mm_alignr_epi8(first_len, previous->first_len, 15));

  /* Third Byte: set range index to saturate_sub(first_len, 1) */
  /* 0 for 00~7F, 0 for C0~DF, 1 for E0~EF, 2 for F0~FF */
  __m128i tmp1, tmp2;
  /* tmp1 = saturate_sub(first_len, 1) */
  tmp1 = _mm_subs_epu8(first_len, _mm_set1_epi8(1));
  /* tmp2 = saturate_sub(prev_first_len, 1) */
  tmp2 = _mm_subs_epu8(previous->first_len, _mm_set1_epi8(1));
  /* range |= (tmp1, tmp2) << 2 bytes */
  range = _mm_or_si128(range, _mm_alignr_epi8(tmp1, tmp2, 14));

  /* Fourth Byte: set range index to saturate_sub(first_len, 2) */
  /* 0 for 00~7F, 0 for C0~DF, 0 for E0~EF, 1 for F0~FF */
  /* tmp1 = saturate_sub(first_len, 2) */
  tmp1 = _mm_subs_epu8(first_len, _mm_set1_epi8(2));
  /* tmp2 = saturate_sub(prev_first_len, 2) */
  tmp2 = _mm_subs_epu8(previous->first_len, _mm_set1_epi8(2));
  /* range |= (tmp1, tmp2) << 3 bytes */
  range = _mm_or_si128(range, _mm_alignr_epi8(tmp1, tmp2, 13));

  /*
   * Now we have below range indices caluclated
   * Correct cases:
   * - 8 for C0~FF
   * - 3 for 1st byte after F0~FF
   * - 2 for 1st byte after E0~EF or 2nd byte after F0~FF
   * - 1 for 1st byte after C0~DF or 2nd byte after E0~EF or
   *         3rd byte after F0~FF
   * - 0 for others
   * Error cases:
   *   9,10,11 if non ascii First Byte overlaps
   *   E.g., F1 80 C2 90 --> 8 3 10 2, where 10 indicates error
   */

  /* Adjust Second Byte range for special First Bytes(E0,ED,F0,F4) */
  /* Overlaps lead to index 9~15, which are illegal in range table */
  __m128i shift1, pos, range2;
  /* shift1 = (input, prev_input) << 1 byte */
  shift1 = _mm_alignr_epi8(input, previous->input, 15);
  pos = _mm_sub_epi8(shift1, _mm_set1_epi8(0xEF));
  /*
   * shift1:  | EF  F0 ... FE | FF  00  ... ...  DE | DF  E0 ... EE |
   * pos:     | 0   1      15 | 16  17           239| 240 241    255|
   * pos-240: | 0   0      0  | 0   0            0  | 0   1      15 |
   * pos+112: | 112 113    127|       >= 128        |     >= 128    |
   */
  tmp1 = _mm_subs_epu8(pos, _mm_set1_epi8(240));
  range2 = _mm_shuffle_epi8(df_ee_tbl, tmp1);
  tmp2 = _mm_adds_epu8(pos, _mm_set1_epi8(112));
  range2 = _mm_add_epi8(range2, _mm_shuffle_epi8(ef_fe_tbl, tmp2));

  range = _mm_add_epi8(range, range2);

  /* Load min and max values per calculated range index */
  __m128i minv = _mm_shuffle_epi8(range_min_tbl, range);
  __m128i maxv = _mm_shuffle_epi8(range_max_tbl, range);

  *has_error = _mm_or_si128(*has_error, _mm_cmplt_epi8(input, minv));
  *has_error = _mm_or_si128(*has_error, _mm_cmpgt_epi8(input, maxv));
  previous->input = input;
  previous->first_len = first_len;

  return *previous;
}

template <>
struct utf8_checking_state<Architecture::WESTMERE> {
  __m128i has_error = _mm_setzero_si128();
  processed_utf_bytes previous{
      _mm_setzero_si128(), // input
      _mm_setzero_si128(), // first_len
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
        _mm_or_si128(_mm_cmpgt_epi8(state.previous.first_len,
                                    _mm_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                                  9, 9, 9, 3, 2, 1)),
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
        _mm_or_si128(_mm_cmpgt_epi8(state.previous.first_len,
                                    _mm_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                                  9, 9, 9, 3, 2, 1)),
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

} // namespace simdjson
UNTARGET_REGION // westmere

#endif // IS_X86_64

#endif
