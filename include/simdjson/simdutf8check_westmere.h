#ifndef SIMDJSON_SIMDUTF8CHECK_WESTMERE_H
#define SIMDJSON_SIMDUTF8CHECK_WESTMERE_H

#include "simdjson/portability.h"
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

/********** sse code **********/
TARGET_WESTMERE

namespace simdjson {
// all byte values must be no larger than 0xF4
static inline void check_smaller_than_0xF4(__m128i current_bytes,
                                           __m128i *has_error) {
  // unsigned, saturates to 0 below max
  *has_error = _mm_or_si128(*has_error,
                            _mm_subs_epu8(current_bytes, _mm_set1_epi8(0xF4u)));
}

static inline __m128i continuation_lengths(__m128i high_nibbles) {
  return _mm_shuffle_epi8(
      _mm_setr_epi8(1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
                    0, 0, 0, 0,             // 10xx (continuation)
                    2, 2,                   // 110x
                    3,                      // 1110
                    4), // 1111, next should be 0 (not checked here)
      high_nibbles);
}

static inline __m128i carry_continuations(__m128i initial_lengths,
                                          __m128i previous_carries) {

  __m128i right1 =
      _mm_subs_epu8(_mm_alignr_epi8(initial_lengths, previous_carries, 16 - 1),
                    _mm_set1_epi8(1));
  __m128i sum = _mm_add_epi8(initial_lengths, right1);

  __m128i right2 = _mm_subs_epu8(_mm_alignr_epi8(sum, previous_carries, 16 - 2),
                                 _mm_set1_epi8(2));
  return _mm_add_epi8(sum, right2);
}

static inline void check_continuations(__m128i initial_lengths, __m128i carries,
                                       __m128i *has_error) {

  // overlap || underlap
  // carry > length && length > 0 || !(carry > length) && !(length > 0)
  // (carries > length) == (lengths > 0)
  __m128i overunder =
      _mm_cmpeq_epi8(_mm_cmpgt_epi8(carries, initial_lengths),
                     _mm_cmpgt_epi8(initial_lengths, _mm_setzero_si128()));

  *has_error = _mm_or_si128(*has_error, overunder);
}

// when 0xED is found, next byte must be no larger than 0x9F
// when 0xF4 is found, next byte must be no larger than 0x8F
// next byte must be continuation, ie sign bit is set, so signed < is ok
static inline void check_first_continuation_max(__m128i current_bytes,
                                                __m128i off1_current_bytes,
                                                __m128i *has_error) {
  __m128i maskED = _mm_cmpeq_epi8(off1_current_bytes, _mm_set1_epi8(0xEDu));
  __m128i maskF4 = _mm_cmpeq_epi8(off1_current_bytes, _mm_set1_epi8(0xF4u));

  __m128i badfollowED = _mm_and_si128(
      _mm_cmpgt_epi8(current_bytes, _mm_set1_epi8(0x9Fu)), maskED);
  __m128i badfollowF4 = _mm_and_si128(
      _mm_cmpgt_epi8(current_bytes, _mm_set1_epi8(0x8Fu)), maskF4);

  *has_error = _mm_or_si128(*has_error, _mm_or_si128(badfollowED, badfollowF4));
}

// map off1_hibits => error condition
// hibits     off1    cur
// C       => < C2 && true
// E       => < E1 && < A0
// F       => < F1 && < 90
// else      false && false
static inline void check_overlong(__m128i current_bytes,
                                  __m128i off1_current_bytes, __m128i hibits,
                                  __m128i previous_hibits, __m128i *has_error) {
  __m128i off1_hibits = _mm_alignr_epi8(hibits, previous_hibits, 16 - 1);
  __m128i initial_mins = _mm_shuffle_epi8(
      _mm_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
                    -128, -128,  // 10xx => false
                    0xC2u, -128, // 110x
                    0xE1u,       // 1110
                    0xF1u),
      off1_hibits);

  __m128i initial_under = _mm_cmpgt_epi8(initial_mins, off1_current_bytes);

  __m128i second_mins = _mm_shuffle_epi8(
      _mm_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
                    -128, -128, // 10xx => false
                    127, 127,   // 110x => true
                    0xA0u,      // 1110
                    0x90u),
      off1_hibits);
  __m128i second_under = _mm_cmpgt_epi8(second_mins, current_bytes);
  *has_error =
      _mm_or_si128(*has_error, _mm_and_si128(initial_under, second_under));
}

struct processed_utf_bytes {
  __m128i raw_bytes;
  __m128i high_nibbles;
  __m128i carried_continuations;
};

static inline void count_nibbles(__m128i bytes,
                                 struct processed_utf_bytes *answer) {
  answer->raw_bytes = bytes;
  answer->high_nibbles =
      _mm_and_si128(_mm_srli_epi16(bytes, 4), _mm_set1_epi8(0x0F));
}

// check whether the current bytes are valid UTF-8
// at the end of the function, previous gets updated
static struct processed_utf_bytes
check_utf8_bytes(__m128i current_bytes, struct processed_utf_bytes *previous,
                 __m128i *has_error) {
  struct processed_utf_bytes pb;
  count_nibbles(current_bytes, &pb);

  check_smaller_than_0xF4(current_bytes, has_error);

  __m128i initial_lengths = continuation_lengths(pb.high_nibbles);

  pb.carried_continuations =
      carry_continuations(initial_lengths, previous->carried_continuations);

  check_continuations(initial_lengths, pb.carried_continuations, has_error);

  __m128i off1_current_bytes =
      _mm_alignr_epi8(pb.raw_bytes, previous->raw_bytes, 16 - 1);
  check_first_continuation_max(current_bytes, off1_current_bytes, has_error);

  check_overlong(current_bytes, off1_current_bytes, pb.high_nibbles,
                 previous->high_nibbles, has_error);
  return pb;
}
} // namespace simdjson
UNTARGET_REGION // westmere

#endif // IS_X86_64

#endif
