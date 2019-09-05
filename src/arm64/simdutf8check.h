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

namespace simdjson::arm64 {
namespace { // private namespace
/*
 * Map high nibble of "First Byte" to legal character length minus 1
 * 0x00 ~ 0xBF --> 0
 * 0xC0 ~ 0xDF --> 1
 * 0xE0 ~ 0xEF --> 2
 * 0xF0 ~ 0xFF --> 3
 */
static const uint8_t _first_len_tbl[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3,
};

/* Map "First Byte" to 8-th item of range table (0xC2 ~ 0xF4) */
static const uint8_t _first_range_tbl[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8,
};

/*
 * Range table, map range index to min and max values
 * Index 0    : 00 ~ 7F (First Byte, ascii)
 * Index 1,2,3: 80 ~ BF (Second, Third, Fourth Byte)
 * Index 4    : A0 ~ BF (Second Byte after E0)
 * Index 5    : 80 ~ 9F (Second Byte after ED)
 * Index 6    : 90 ~ BF (Second Byte after F0)
 * Index 7    : 80 ~ 8F (Second Byte after F4)
 * Index 8    : C2 ~ F4 (First Byte, non ascii)
 * Index 9~15 : illegal: u >= 255 && u <= 0
 */
static const uint8_t _range_min_tbl[] = {
    0x00, 0x80, 0x80, 0x80, 0xA0, 0x80, 0x90, 0x80,
    0xC2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
static const uint8_t _range_max_tbl[] = {
    0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x9F, 0xBF, 0x8F,
    0xF4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/*
 * This table is for fast handling four special First Bytes(E0,ED,F0,F4), after
 * which the Second Byte are not 80~BF. It contains "range index adjustment".
 * - The idea is to minus byte with E0, use the result(0~31) as the index to
 *   lookup the "range index adjustment". Then add the adjustment to original
 *   range index to get the correct range.
 * - Range index adjustment
 *   +------------+---------------+------------------+----------------+
 *   | First Byte | original range| range adjustment | adjusted range |
 *   +------------+---------------+------------------+----------------+
 *   | E0         | 2             | 2                | 4              |
 *   +------------+---------------+------------------+----------------+
 *   | ED         | 2             | 3                | 5              |
 *   +------------+---------------+------------------+----------------+
 *   | F0         | 3             | 3                | 6              |
 *   +------------+---------------+------------------+----------------+
 *   | F4         | 4             | 4                | 8              |
 *   +------------+---------------+------------------+----------------+
 * - Below is a uint8x16x2 table, data is interleaved in NEON register. So I'm
 *   putting it vertically. 1st column is for E0~EF, 2nd column for F0~FF.
 */
static const uint8_t _range_adjust_tbl[] = {
    /* index -> 0~15  16~31 <- index */
    /*  E0 -> */ 2,     3, /* <- F0  */
                 0,     0,
                 0,     0,
                 0,     0,
                 0,     4, /* <- F4  */
                 0,     0,
                 0,     0,
                 0,     0,
                 0,     0,
                 0,     0,
                 0,     0,
                 0,     0,
                 0,     0,
    /*  ED -> */ 3,     0,
                 0,     0,
                 0,     0,
};
} // private namespace

struct processed_utf_bytes {
  uint8x16_t input;
  uint8x16_t first_len;
};

// check whether the current bytes are valid UTF-8
// at the end of the function, previous gets updated
static inline struct processed_utf_bytes
check_utf8_bytes(uint8x16_t input,
                 struct processed_utf_bytes *previous,
                 uint8x16_t *has_error) {
    /* Cached tables */
    const uint8x16_t first_len_tbl = vld1q_u8(_first_len_tbl);
    const uint8x16_t first_range_tbl = vld1q_u8(_first_range_tbl);
    const uint8x16_t range_min_tbl = vld1q_u8(_range_min_tbl);
    const uint8x16_t range_max_tbl = vld1q_u8(_range_max_tbl);
    const uint8x16x2_t range_adjust_tbl = vld2q_u8(_range_adjust_tbl);

    /* Cached values */
    const uint8x16_t const_1 = vdupq_n_u8(1);
    const uint8x16_t const_2 = vdupq_n_u8(2);
    const uint8x16_t const_e0 = vdupq_n_u8(0xE0);

    /* high_nibbles = input >> 4 */
    const uint8x16_t high_nibbles = vshrq_n_u8(input, 4);

    /* first_len = legal character length minus 1 */
    /* 0 for 00~7F, 1 for C0~DF, 2 for E0~EF, 3 for F0~FF */
    /* first_len = first_len_tbl[high_nibbles] */
    const uint8x16_t first_len =
        vqtbl1q_u8(first_len_tbl, high_nibbles);

    /* First Byte: set range index to 8 for bytes within 0xC0 ~ 0xFF */
    /* range = first_range_tbl[high_nibbles] */
    uint8x16_t range = vqtbl1q_u8(first_range_tbl, high_nibbles);

    /* Second Byte: set range index to first_len */
    /* 0 for 00~7F, 1 for C0~DF, 2 for E0~EF, 3 for F0~FF */
    /* range |= (first_len, previous->first_len) << 1 byte */
    range =
        vorrq_u8(range, vextq_u8(previous->first_len, first_len, 15));

    /* Third Byte: set range index to saturate_sub(first_len, 1) */
    /* 0 for 00~7F, 0 for C0~DF, 1 for E0~EF, 2 for F0~FF */
    uint8x16_t tmp1, tmp2;
    /* tmp1 = saturate_sub(first_len, 1) */
    tmp1 = vqsubq_u8(first_len, const_1);
    /* tmp2 = saturate_sub(previous->first_len, 1) */
    tmp2 = vqsubq_u8(previous->first_len, const_1);
    /* range |= (tmp1, tmp2) << 2 bytes */
    range = vorrq_u8(range, vextq_u8(tmp2, tmp1, 14));

    /* Fourth Byte: set range index to saturate_sub(first_len, 2) */
    /* 0 for 00~7F, 0 for C0~DF, 0 for E0~EF, 1 for F0~FF */
    /* tmp1 = saturate_sub(first_len, 2) */
    tmp1 = vqsubq_u8(first_len, const_2);
    /* tmp2 = saturate_sub(prev_first_len, 2) */
    tmp2 = vqsubq_u8(previous->first_len, const_2);
    /* range |= (tmp1, tmp2) << 3 bytes */
    range = vorrq_u8(range, vextq_u8(tmp2, tmp1, 13));

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
    /* See _range_adjust_tbl[] definition for details */
    /* Overlaps lead to index 9~15, which are illegal in range table */
    uint8x16_t shift1 = vextq_u8(previous->input, input, 15);
    uint8x16_t pos = vsubq_u8(shift1, const_e0);
    range = vaddq_u8(range, vqtbl2q_u8(range_adjust_tbl, pos));

    /* Load min and max values per calculated range index */
    uint8x16_t minv = vqtbl1q_u8(range_min_tbl, range);
    uint8x16_t maxv = vqtbl1q_u8(range_max_tbl, range);

    /* Check value range */
    *has_error = vorrq_u8(*has_error, vcltq_u8(input, minv));
    *has_error = vorrq_u8(*has_error, vcgtq_u8(input, maxv));

    previous->input = input;
    previous->first_len = first_len;

    return *previous;
}

// Checks that all bytes are ascii
really_inline bool check_ascii_neon(simd_input<Architecture::ARM64> in) {
  // checking if the most significant bit is always equal to 0.
  uint8x16_t high_bit = vdupq_n_u8(0x80);
  uint8x16_t any_bits_on = in.reduce([&](auto a, auto b) {
    return vorrq_u8(a, b);
  });
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
  uint8x16_t has_error{};
  processed_utf_bytes previous{};

  really_inline void check_next_input(simd_input<Architecture::ARM64> in) {
    if (check_ascii_neon(in)) {
      // All bytes are ascii. Therefore the byte that was just before must be
      // ascii too. We only check the byte that was just before simd_input. Nines
      // are arbitrary values.
      const uint8x16_t verror =
          (uint8x16_t){9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 1, 0};
      this->has_error =
          vorrq_u8(vcgtq_u8(this->previous.first_len, verror),
                   this->has_error);
    } else {
      // it is not ascii so we have to do heavy work
      in.each([&](auto _in) {
        this->previous = check_utf8_bytes(_in, &(this->previous), &(this->has_error));
      });
    }
  }

  really_inline ErrorValues errors() {
    uint64x2_t v64 = vreinterpretq_u64_u8(this->has_error);
    uint32x2_t v32 = vqmovn_u64(v64);
    uint64x1_t result = vreinterpret_u64_u32(v32);
    return vget_lane_u64(result, 0) != 0 ? simdjson::UTF8_ERROR
                                        : simdjson::SUCCESS;
  }

}; // struct utf8_checker

} // namespace simdjson
#endif
#endif
