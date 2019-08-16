#ifndef SIMDJSON_SIMDUTF8CHECK_HASWELL_H
#define SIMDJSON_SIMDUTF8CHECK_HASWELL_H

#include "simdjson/portability.h"
#include "simdjson/simdutf8check.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef IS_X86_64
/*
 * legal utf-8 byte sequence
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
 *
 *  Code Points        1st       2s       3s       4s
 * U+0000..U+007F     00..7F
 * U+0080..U+07FF     C2..DF   80..BF
 * U+0800..U+0FFF     E0       A0..BF   80..BF
 * U+1000..U+CFFF     E1..EC   80..BF   80..BF
 * U+D000..U+D7FF     ED       80..9F   80..BF
 * U+E000..U+FFFF     EE..EF   80..BF   80..BF
 * U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
 * U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
 * U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
 *
 */

// all byte values must be no larger than 0xF4

TARGET_HASWELL
namespace simdjson {
static inline __m256i push_last_byte_of_a_to_b(__m256i a, __m256i b) {
  return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 15);
}

static inline __m256i push_last_2bytes_of_a_to_b(__m256i a, __m256i b) {
  return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 14);
}

static inline __m256i push_last_3bytes_of_a_to_b(__m256i a, __m256i b) {
  return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 13);
}

struct avx_processed_utf_bytes {
  __m256i input;
  __m256i first_len;
};

// check whether the current bytes are valid UTF-8
// at the end of the function, previous gets updated
static inline struct avx_processed_utf_bytes
avx_check_utf8_bytes(__m256i input,
                     struct avx_processed_utf_bytes *previous,
                     __m256i *has_error) {
  /*
  * Map high nibble of "First Byte" to legal character length minus 1
  * 0x00 ~ 0xBF --> 0
  * 0xC0 ~ 0xDF --> 1
  * 0xE0 ~ 0xEF --> 2
  * 0xF0 ~ 0xFF --> 3
  */
  const __m256i first_len_tbl =
        _mm256_setr_epi8(
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3,
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3);

  /* Map "First Byte" to 8-th item of range table (0xC2 ~ 0xF4) */
  const __m256i first_range_tbl =
        _mm256_setr_epi8(
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8,
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8);

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
  const __m256i range_min_tbl = 
        _mm256_setr_epi8(
          0x00, 0x80, 0x80, 0x80, 0xA0, 0x80, 0x90, 0x80,
          0xC2, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x00, 0x80, 0x80, 0x80, 0xA0, 0x80, 0x90, 0x80,
          0xC2, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F);
  const __m256i range_max_tbl = 
        _mm256_setr_epi8(
          0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x9F, 0xBF, 0x8F,
          0xF4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
          0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x9F, 0xBF, 0x8F,
          0xF4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);

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
  const __m256i df_ee_tbl = 
        _mm256_setr_epi8(
          0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,
          0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0);
  /* index1 -> F0, index5 -> F4 */
  const __m256i ef_fe_tbl = 
        _mm256_setr_epi8(
          0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  /* high_nibbles = input >> 4 */
  const __m256i high_nibbles =
      _mm256_and_si256(_mm256_srli_epi16(input, 4), _mm256_set1_epi8(0x0F));

  /* first_len = legal character length minus 1 */
  /* 0 for 00~7F, 1 for C0~DF, 2 for E0~EF, 3 for F0~FF */
  /* first_len = first_len_tbl[high_nibbles] */
  __m256i first_len = _mm256_shuffle_epi8(first_len_tbl, high_nibbles);

  /* First Byte: set range index to 8 for bytes within 0xC0 ~ 0xFF */
  /* range = first_range_tbl[high_nibbles] */
  __m256i range = _mm256_shuffle_epi8(first_range_tbl, high_nibbles);

  /* Second Byte: set range index to first_len */
  /* 0 for 00~7F, 1 for C0~DF, 2 for E0~EF, 3 for F0~FF */
  /* range |= (first_len, previous->first_len) << 1 byte */
  range = _mm256_or_si256(
          range, push_last_byte_of_a_to_b(previous->first_len, first_len));

  /* Third Byte: set range index to saturate_sub(first_len, 1) */
  /* 0 for 00~7F, 0 for C0~DF, 1 for E0~EF, 2 for F0~FF */
  __m256i tmp1, tmp2;

  /* tmp1 = saturate_sub(first_len, 1) */
  tmp1 = _mm256_subs_epu8(first_len, _mm256_set1_epi8(1));
  /* tmp2 = saturate_sub(previous->first_len, 1) */
  tmp2 = _mm256_subs_epu8(previous->first_len, _mm256_set1_epi8(1));

  /* range |= (tmp1, tmp2) << 2 bytes */
  range = _mm256_or_si256(range, push_last_2bytes_of_a_to_b(tmp2, tmp1));

  /* Fourth Byte: set range index to saturate_sub(first_len, 2) */
  /* 0 for 00~7F, 0 for C0~DF, 0 for E0~EF, 1 for F0~FF */
  /* tmp1 = saturate_sub(first_len, 2) */
  tmp1 = _mm256_subs_epu8(first_len, _mm256_set1_epi8(2));
  /* tmp2 = saturate_sub(previous->first_len, 2) */
  tmp2 = _mm256_subs_epu8(previous->first_len, _mm256_set1_epi8(2));
  /* range |= (tmp1, tmp2) << 3 bytes */
  range = _mm256_or_si256(range, push_last_3bytes_of_a_to_b(tmp2, tmp1));

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
  __m256i shift1, pos, range2;
  /* shift1 = (input, previous->input) << 1 byte */
  shift1 = push_last_byte_of_a_to_b(previous->input, input);
  pos = _mm256_sub_epi8(shift1, _mm256_set1_epi8(0xEF));
  /*
    * shift1:  | EF  F0 ... FE | FF  00  ... ...  DE | DF  E0 ... EE |
    * pos:     | 0   1      15 | 16  17           239| 240 241    255|
    * pos-240: | 0   0      0  | 0   0            0  | 0   1      15 |
    * pos+112: | 112 113    127|       >= 128        |     >= 128    |
    */
  tmp1 = _mm256_subs_epu8(pos, _mm256_set1_epi8(240));
  range2 = _mm256_shuffle_epi8(df_ee_tbl, tmp1);
  tmp2 = _mm256_adds_epu8(pos, _mm256_set1_epi8(112));
  range2 = _mm256_add_epi8(range2, _mm256_shuffle_epi8(ef_fe_tbl, tmp2));

  range = _mm256_add_epi8(range, range2);

  /* Load min and max values per calculated range index */
  __m256i minv = _mm256_shuffle_epi8(range_min_tbl, range);
  __m256i maxv = _mm256_shuffle_epi8(range_max_tbl, range);

  *has_error = _mm256_or_si256(*has_error, _mm256_cmpgt_epi8(minv, input));
  *has_error = _mm256_or_si256(*has_error, _mm256_cmpgt_epi8(input, maxv));

  previous->input = input;
  previous->first_len = first_len;

  return *previous;
}

template <> struct utf8_checking_state<Architecture::HASWELL> {
  __m256i has_error;
  avx_processed_utf_bytes previous;
  utf8_checking_state() {
    has_error = _mm256_setzero_si256();
    previous.input = _mm256_setzero_si256();
    previous.first_len = _mm256_setzero_si256();
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
        _mm256_cmpgt_epi8(state.previous.first_len,
                          _mm256_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                           9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                           9, 9, 9, 9, 9, 2, 1, 0)),
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

} // namespace simdjson
UNTARGET_REGION // haswell

#endif // IS_X86_64

#endif
