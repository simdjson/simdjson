// From https://github.com/cyb70289/utf8/blob/master/lemire-neon.c
// Adapted from https://github.com/lemire/fastvalidate-utf-8

#ifndef SIMDJSON_ARM64_SIMDUTF8CHECK_H
#define SIMDJSON_ARM64_SIMDUTF8CHECK_H

#if defined(_ARM_NEON) || defined(__aarch64__) ||                              \
    (defined(_MSC_VER) && defined(_M_ARM64))

#include "../simdutf8check.h"
#include <arm_neon.h>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

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
namespace simdjson::arm64 {

// all byte values must be no larger than 0xF4
static inline void check_smaller_than_0xF4(int8x16_t current_bytes,
                                           int8x16_t *has_error) {
  // unsigned, saturates to 0 below max
  *has_error = vorrq_s8(
      *has_error, vreinterpretq_s8_u8(vqsubq_u8(
                      vreinterpretq_u8_s8(current_bytes), vdupq_n_u8(0xF4))));
}

static const int8_t _nibbles[] = {
    1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
    0, 0, 0, 0,             // 10xx (continuation)
    2, 2,                   // 110x
    3,                      // 1110
    4,                      // 1111, next should be 0 (not checked here)
};

static inline int8x16_t continuation_lengths(int8x16_t high_nibbles) {
  return vqtbl1q_s8(vld1q_s8(_nibbles), vreinterpretq_u8_s8(high_nibbles));
}

static inline int8x16_t carry_continuations(int8x16_t initial_lengths,
                                            int8x16_t previous_carries) {

  int8x16_t right1 = vreinterpretq_s8_u8(vqsubq_u8(
      vreinterpretq_u8_s8(vextq_s8(previous_carries, initial_lengths, 16 - 1)),
      vdupq_n_u8(1)));
  int8x16_t sum = vaddq_s8(initial_lengths, right1);

  int8x16_t right2 = vreinterpretq_s8_u8(
      vqsubq_u8(vreinterpretq_u8_s8(vextq_s8(previous_carries, sum, 16 - 2)),
                vdupq_n_u8(2)));
  return vaddq_s8(sum, right2);
}

static inline void check_continuations(int8x16_t initial_lengths,
                                       int8x16_t carries,
                                       int8x16_t *has_error) {

  // overlap || underlap
  // carry > length && length > 0 || !(carry > length) && !(length > 0)
  // (carries > length) == (lengths > 0)
  uint8x16_t overunder = vceqq_u8(vcgtq_s8(carries, initial_lengths),
                                  vcgtq_s8(initial_lengths, vdupq_n_s8(0)));

  *has_error = vorrq_s8(*has_error, vreinterpretq_s8_u8(overunder));
}

// when 0xED is found, next byte must be no larger than 0x9F
// when 0xF4 is found, next byte must be no larger than 0x8F
// next byte must be continuation, ie sign bit is set, so signed < is ok
static inline void check_first_continuation_max(int8x16_t current_bytes,
                                                int8x16_t off1_current_bytes,
                                                int8x16_t *has_error) {
  uint8x16_t maskED = vceqq_s8(off1_current_bytes, vdupq_n_s8(0xED));
  uint8x16_t maskF4 = vceqq_s8(off1_current_bytes, vdupq_n_s8(0xF4));

  uint8x16_t badfollowED =
      vandq_u8(vcgtq_s8(current_bytes, vdupq_n_s8(0x9F)), maskED);
  uint8x16_t badfollowF4 =
      vandq_u8(vcgtq_s8(current_bytes, vdupq_n_s8(0x8F)), maskF4);

  *has_error = vorrq_s8(
      *has_error, vreinterpretq_s8_u8(vorrq_u8(badfollowED, badfollowF4)));
}

static const int8_t _initial_mins[] = {
    -128,         -128, -128, -128, -128, -128,
    -128,         -128, -128, -128, -128, -128, // 10xx => false
    (int8_t)0xC2, -128,                         // 110x
    (int8_t)0xE1,                               // 1110
    (int8_t)0xF1,
};

static const int8_t _second_mins[] = {
    -128,         -128, -128, -128, -128, -128,
    -128,         -128, -128, -128, -128, -128, // 10xx => false
    127,          127,                          // 110x => true
    (int8_t)0xA0,                               // 1110
    (int8_t)0x90,
};

// map off1_hibits => error condition
// hibits     off1    cur
// C       => < C2 && true
// E       => < E1 && < A0
// F       => < F1 && < 90
// else      false && false
static inline void check_overlong(int8x16_t current_bytes,
                                  int8x16_t off1_current_bytes,
                                  int8x16_t hibits, int8x16_t previous_hibits,
                                  int8x16_t *has_error) {
  int8x16_t off1_hibits = vextq_s8(previous_hibits, hibits, 16 - 1);
  int8x16_t initial_mins =
      vqtbl1q_s8(vld1q_s8(_initial_mins), vreinterpretq_u8_s8(off1_hibits));

  uint8x16_t initial_under = vcgtq_s8(initial_mins, off1_current_bytes);

  int8x16_t second_mins =
      vqtbl1q_s8(vld1q_s8(_second_mins), vreinterpretq_u8_s8(off1_hibits));
  uint8x16_t second_under = vcgtq_s8(second_mins, current_bytes);
  *has_error = vorrq_s8(
      *has_error, vreinterpretq_s8_u8(vandq_u8(initial_under, second_under)));
}

struct processed_utf_bytes {
  int8x16_t raw_bytes;
  int8x16_t high_nibbles;
  int8x16_t carried_continuations;
};

static inline void count_nibbles(int8x16_t bytes,
                                 struct processed_utf_bytes *answer) {
  answer->raw_bytes = bytes;
  answer->high_nibbles =
      vreinterpretq_s8_u8(vshrq_n_u8(vreinterpretq_u8_s8(bytes), 4));
}

// check whether the current bytes are valid UTF-8
// at the end of the function, previous gets updated
static inline struct processed_utf_bytes
check_utf8_bytes(int8x16_t current_bytes, struct processed_utf_bytes *previous,
                 int8x16_t *has_error) {
  struct processed_utf_bytes pb;
  count_nibbles(current_bytes, &pb);

  check_smaller_than_0xF4(current_bytes, has_error);

  int8x16_t initial_lengths = continuation_lengths(pb.high_nibbles);

  pb.carried_continuations =
      carry_continuations(initial_lengths, previous->carried_continuations);

  check_continuations(initial_lengths, pb.carried_continuations, has_error);

  int8x16_t off1_current_bytes =
      vextq_s8(previous->raw_bytes, pb.raw_bytes, 16 - 1);
  check_first_continuation_max(current_bytes, off1_current_bytes, has_error);

  check_overlong(current_bytes, off1_current_bytes, pb.high_nibbles,
                 previous->high_nibbles, has_error);
  return pb;
}

// Checks that all bytes are ascii
really_inline bool check_ascii_neon(simd_input<Architecture::ARM64> in) {
  // checking if the most significant bit is always equal to 0.
  uint8x16_t high_bit = vdupq_n_u8(0x80);
  uint8x16_t any_bits_on = REDUCE_CHUNKS(in, vorrq_u8(_a, _b));
  uint8x16_t high_bit_on = vandq_u8(any_bits_on, high_bit);
  uint64x2_t v64 = vreinterpretq_u64_u8(high_bit_on);
  uint32x2_t v32 = vqmovn_u64(v64);
  uint64x1_t result = vreinterpret_u64_u32(v32);
  return vget_lane_u64(result, 0) == 0;
}

} // namespace simdjson::arm64

namespace simdjson {

using namespace simdjson::arm64;

template <>
struct utf8_checker<Architecture::ARM64> {
  int8x16_t has_error{};
  processed_utf_bytes previous{};

  really_inline void check_next_input(simd_input<Architecture::ARM64> in) {
    if (check_ascii_neon(in)) {
      // All bytes are ascii. Therefore the byte that was just before must be
      // ascii too. We only check the byte that was just before simd_input. Nines
      // are arbitrary values.
      const int8x16_t verror =
          (int8x16_t){9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 1};
      this->has_error =
          vorrq_s8(vreinterpretq_s8_u8(
                      vcgtq_s8(this->previous.carried_continuations, verror)),
                  this->has_error);
    } else {
      // it is not ascii so we have to do heavy work
      in.each([&](auto _in) {
        this->previous = check_utf8_bytes(vreinterpretq_s8_u8(_in), &(this->previous), &(this->has_error));
      });
    }
  }

  really_inline ErrorValues errors() {
    uint64x2_t v64 = vreinterpretq_u64_s8(this->has_error);
    uint32x2_t v32 = vqmovn_u64(v64);
    uint64x1_t result = vreinterpret_u64_u32(v32);
    return vget_lane_u64(result, 0) != 0 ? simdjson::UTF8_ERROR
                                        : simdjson::SUCCESS;
  }

}; // struct utf8_checker

} // namespace simdjson
#endif
#endif
