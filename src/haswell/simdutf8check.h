#ifndef SIMDJSON_HASWELL_SIMDUTF8CHECK_H
#define SIMDJSON_HASWELL_SIMDUTF8CHECK_H

#include "simdjson/portability.h"
#include "simdjson/simdjson.h"
#include "haswell/simd_input.h"
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
namespace simdjson::haswell {

static inline __m256i push_last_byte_of_a_to_b(__m256i a, __m256i b) {
  return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 15);
}

static inline __m256i push_last_2bytes_of_a_to_b(__m256i a, __m256i b) {
  return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 14);
}

struct processed_utf_bytes {
  __m256i raw_bytes;
  __m256i high_nibbles;
  __m256i carried_continuations;
};

struct utf8_checker {
  __m256i has_error;
  processed_utf_bytes previous;

  utf8_checker() :
      has_error{_mm256_setzero_si256()},
      previous{_mm256_setzero_si256(), _mm256_setzero_si256(), _mm256_setzero_si256()} {}

  really_inline void add_errors(__m256i errors) {
    this->has_error = _mm256_or_si256(this->has_error, errors);
  }

  // all byte values must be no larger than 0xF4
  really_inline void check_smaller_than_0xF4(__m256i current_bytes) {
    // unsigned, saturates to 0 below max
    this->add_errors( _mm256_subs_epu8(current_bytes, _mm256_set1_epi8(0xF4u)) );
  }

  really_inline __m256i continuation_lengths(__m256i high_nibbles) {
    return _mm256_shuffle_epi8(
        _mm256_setr_epi8(1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
                         0, 0, 0, 0,             // 10xx (continuation)
                         2, 2,                   // 110x
                         3,                      // 1110
                         4, // 1111, next should be 0 (not checked here)
                         1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
                         0, 0, 0, 0,             // 10xx (continuation)
                         2, 2,                   // 110x
                         3,                      // 1110
                         4), // 1111, next should be 0 (not checked here)
                         
        high_nibbles);
  }

  really_inline __m256i carry_continuations(__m256i initial_lengths) {
    __m256i right1 = _mm256_subs_epu8(
        push_last_byte_of_a_to_b(this->previous.carried_continuations, initial_lengths),
        _mm256_set1_epi8(1));
    __m256i sum = _mm256_add_epi8(initial_lengths, right1);

    __m256i right2 = _mm256_subs_epu8(
        push_last_2bytes_of_a_to_b(this->previous.carried_continuations, sum), _mm256_set1_epi8(2));
    return _mm256_add_epi8(sum, right2);
  }

  really_inline void check_continuations(__m256i initial_lengths, __m256i carries) {
    // overlap || underlap
    // carry > length && length > 0 || !(carry > length) && !(length > 0)
    // (carries > length) == (lengths > 0)
    // (carries > current) == (current > 0)
    __m256i overunder = _mm256_cmpeq_epi8(
        _mm256_cmpgt_epi8(carries, initial_lengths),
        _mm256_cmpgt_epi8(initial_lengths, _mm256_setzero_si256()));

    this->add_errors( overunder );
  }

  really_inline void check_carried_continuations() {
    this->add_errors(
      _mm256_cmpgt_epi8(this->previous.carried_continuations,
      _mm256_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                       9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                       9, 9, 9, 9, 9, 9, 9, 1))
    );
  }

  // when 0xED is found, next byte must be no larger than 0x9F
  // when 0xF4 is found, next byte must be no larger than 0x8F
  // next byte must be continuation, ie sign bit is set, so signed < is ok
  really_inline void check_first_continuation_max(__m256i current_bytes,
                                                      __m256i off1_current_bytes) {
    __m256i maskED =
        _mm256_cmpeq_epi8(off1_current_bytes, _mm256_set1_epi8(0xEDu));
    __m256i maskF4 =
        _mm256_cmpeq_epi8(off1_current_bytes, _mm256_set1_epi8(0xF4u));

    __m256i badfollowED = _mm256_and_si256(
        _mm256_cmpgt_epi8(current_bytes, _mm256_set1_epi8(0x9Fu)), maskED);
    __m256i badfollowF4 = _mm256_and_si256(
        _mm256_cmpgt_epi8(current_bytes, _mm256_set1_epi8(0x8Fu)), maskF4);

    this->add_errors( _mm256_or_si256(badfollowED, badfollowF4) );
  }

  // map off1_hibits => error condition
  // hibits     off1    cur
  // C       => < C2 && true
  // E       => < E1 && < A0
  // F       => < F1 && < 90
  // else      false && false
  really_inline void check_overlong(__m256i current_bytes,
                                        __m256i off1_current_bytes,
                                        __m256i high_nibbles) {
    __m256i off1_high_nibbles = push_last_byte_of_a_to_b(this->previous.high_nibbles, high_nibbles);
    __m256i initial_mins = _mm256_shuffle_epi8(
        _mm256_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, -128,
                         -128, -128, -128, // 10xx => false
                         0xC2u, -128,      // 110x
                         0xE1u,            // 1110
                         0xF1u,            // 1111
                         -128, -128, -128, -128, -128, -128, -128, -128, -128,
                         -128, -128, -128, // 10xx => false
                         0xC2u, -128,      // 110x
                         0xE1u,            // 1110
                         0xF1u),           // 1111
        off1_high_nibbles);

    __m256i initial_under = _mm256_cmpgt_epi8(initial_mins, off1_current_bytes);

    __m256i second_mins = _mm256_shuffle_epi8(
        _mm256_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, -128,
                         -128, -128, -128, // 10xx => false
                         127, 127,         // 110x => true
                         0xA0u,            // 1110
                         0x90u,            // 1111
                         -128, -128, -128, -128, -128, -128, -128, -128, -128,
                         -128, -128, -128, // 10xx => false
                         127, 127,         // 110x => true
                         0xA0u,            // 1110
                         0x90u),           // 1111
        off1_high_nibbles);
    __m256i second_under = _mm256_cmpgt_epi8(second_mins, current_bytes);
    this->add_errors( _mm256_and_si256(initial_under, second_under) );
  }

  really_inline void count_nibbles(__m256i bytes, struct processed_utf_bytes *answer) {
    answer->raw_bytes = bytes;
    answer->high_nibbles = _mm256_and_si256(_mm256_srli_epi16(bytes, 4), _mm256_set1_epi8(0x0F));
  }

  // check whether the current bytes are valid UTF-8
  // at the end of the function, previous gets updated
  really_inline void check_utf8_bytes(__m256i current_bytes) {
    struct processed_utf_bytes pb {};
    this->count_nibbles(current_bytes, &pb);

    this->check_smaller_than_0xF4(current_bytes);

    __m256i initial_lengths = this->continuation_lengths(pb.high_nibbles);

    pb.carried_continuations = this->carry_continuations(initial_lengths);

    this->check_continuations(initial_lengths, pb.carried_continuations);

    __m256i off1_current_bytes =
        push_last_byte_of_a_to_b(this->previous.raw_bytes, pb.raw_bytes);
    this->check_first_continuation_max(current_bytes, off1_current_bytes);

    this->check_overlong(current_bytes, off1_current_bytes, pb.high_nibbles);
    this->previous = pb;
  }

  really_inline void check_next_input(__m256i in) {
    __m256i high_bit = _mm256_set1_epi8(0x80u);
    if (likely(_mm256_testz_si256(in, high_bit) == 1)) {
      this->check_carried_continuations();
    } else {
      this->check_utf8_bytes(in);
    }
  }

  really_inline void check_next_input(simd_input in) {
    __m256i high_bit = _mm256_set1_epi8(0x80u);
    __m256i any_bits_on = in.reduce([&](auto a, auto b) {
      return _mm256_or_si256(a, b);
    });
    if (likely(_mm256_testz_si256(any_bits_on, high_bit) == 1)) {
      // it is ascii, we just check carried continuations.
      this->check_carried_continuations();
    } else {
      // it is not ascii so we have to do heavy work
      in.each([&](auto _in) { check_utf8_bytes(_in); });
    }
  }

  really_inline ErrorValues errors() {
    return _mm256_testz_si256(this->has_error, this->has_error) == 0 ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
  }
}; // struct utf8_checker

}; // namespace simdjson::haswell
UNTARGET_REGION // haswell

#endif // IS_X86_64

#endif // SIMDJSON_HASWELL_SIMDUTF8CHECK_H
