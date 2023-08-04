#ifndef SIMDJSON_SRC_GENERIC_STAGE1_UTF8_LOOKUP4_ALGORITHM_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_UTF8_LOOKUP4_ALGORITHM_H
#include <generic/stage1/base.h>
#include <generic/dom_parser_implementation.h>
#include <simdjson/generic/lookup_table.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace utf8_validation {

using namespace simd;

// Bit 0 = Too Short (lead byte/ASCII followed by lead byte/ASCII)
// Bit 1 = Too Long (ASCII followed by continuation)
// Bit 2 = Overlong 3-byte
// Bit 4 = Surrogate
// Bit 5 = Overlong 2-byte
// Bit 7 = Two Continuations
static constexpr const uint8_t TOO_SHORT   = 1<<0; // 11______ 0_______
                                                   // 11______ 11______
static constexpr const uint8_t TOO_LONG    = 1<<1; // 0_______ 10______
static constexpr const uint8_t OVERLONG_3  = 1<<2; // 11100000 100_____
static constexpr const uint8_t SURROGATE   = 1<<4; // 11101101 101_____
static constexpr const uint8_t OVERLONG_2  = 1<<5; // 1100000_ 10______
static constexpr const uint8_t TWO_CONTS   = 1<<7; // 10______ 10______
// TOO_LARGE is any 4-byte bigger than 11110100 1001____, and any 5+-byte
// We split it into two parts, recognizing 11110101 1000____ and up in TOO_LARGE_1000
static constexpr const uint8_t TOO_LARGE   = 1<<3; // 11110100 1001____ = 4-byte 100-111 01-11...
                                                   // 11110100 101_____
                                                   // 11110101 1001____
                                                   // 11110101 101_____
                                                   // 1111011_ 1001____
                                                   // 1111011_ 101_____
                                                   // 11111___ 1001____ = 5-byte * 01-11...
                                                   // 11111___ 101_____
static constexpr const uint8_t TOO_LARGE_1000 = 1<<6;
                                                   // 11110101 1000____ = 4-byte 101-111 00...
                                                   // 1111011_ 1000____
                                                   // 11111___ 1000____ = 5-byte * 00 ...
simdjson_constinit uint8_t OVERLONG_4  = 1<<6; // 11110000 1000____

simdjson_constinit byte_range ASCII       = { 0b00000000,   0b01111111 };
simdjson_constinit byte_range CONT        = { 0b10000000,   0b10111111 };
simdjson_constinit byte_range LEAD_2      = { 0b11000000,   0b11011111 };
simdjson_constinit byte_range LEAD_3      = { 0b11100000,   0b11101111 };
simdjson_constinit byte_range LEAD_4      = { 0b11110000,   0b11110111 };
simdjson_constinit byte_range LEAD_5_PLUS = { 0b11111000,   0b11111111 };
simdjson_constinit byte_range LEAD        = LEAD_2 | LEAD_3 | LEAD_4 | LEAD_5_PLUS;

simdjson_constinit const byte_classifier BYTE_1{
  {   ASCII,                    TOO_LONG },       // 0_______ 10______
  {   CONT,                     TWO_CONTS },      // 10______ 10______
  {   LEAD,                     TOO_SHORT },      // 11______ 0_______
                                              // 11______ 11______
  {   0b11101101,               SURROGATE },      // 11101101 101_____

  { { 0b11000000, 0b11000001 }, OVERLONG_2 },     // 1100000_ 10______
  {   0b11100000,               OVERLONG_3 },     // 11100000 100_____
  {   0b11110000,               OVERLONG_4 },     // 11110000 1000____
  { { 0b11110100, 0b11111111 }, TOO_LARGE },      // 11110100-1111____ 1001____-101_____
  { { 0b11110101, 0b11111111 }, TOO_LARGE_1000 } // 11110101-1111____ 1000____
};
simdjson_consteval bool bytes_match(byte_range range, uint8_t value) noexcept {
  for (uint8_t i : range) {
    if (BYTE_1[i] != value) { return false; }
  }
  return true;
}
// static_assert(bytes_match(ASCII, TOO_LONG));
// static_assert(bytes_match(CONT, TWO_CONTS));

// static_assert(bytes_match({ 0b11000000, 0b11000001 }, OVERLONG_2 | TOO_SHORT));
// static_assert(bytes_match({ 0b11000010, 0b11011111 }, TOO_SHORT ));

// static_assert(int(BYTE_1[0b11100000]) == int(OVERLONG_3 | TOO_SHORT));
// static_assert(bytes_match({ 0b11100001, 0b11101100 }, TOO_SHORT));
// static_assert(int(BYTE_1[0b11101101]) == int(SURROGATE | TOO_SHORT));
// static_assert(bytes_match({ 0b11101110, 0b11101111 }, TOO_SHORT));

// static_assert(int(BYTE_1[0b11110000]) == int(OVERLONG_4 | TOO_SHORT));
// static_assert(bytes_match({ 0b11110001, 0b11110011 }, TOO_SHORT));
// static_assert(int(BYTE_1[0b11110100]) == int(TOO_SHORT | TOO_LARGE));
// static_assert(bytes_match({ 0b11110101, 0b11111111 }, TOO_SHORT | TOO_LARGE | TOO_LARGE_1000));
// static_assert(int(BYTE_1[0xf8]) & TOO_LARGE);

static constexpr const high_nibble_lookup BYTE_2_HIGH{
  { CONT,                       TOO_LONG },       // 0_______ 10______
  { CONT,                       TWO_CONTS },      // 10______ 10______
  { ASCII,                      TOO_SHORT },      // 11______ 0_______
  { LEAD,                       TOO_SHORT },      // 11______ 11______
  { { 0b10100000, 0b10111111 }, SURROGATE },      // 11101101 101_____

  { CONT,                       OVERLONG_2 },     // 1100000_ 10______
  { { 0b10000000, 0b10011111 }, OVERLONG_3 },     // 11100000 100_____
  { { 0b10000000, 0b10001111 }, OVERLONG_4 },     // 11110000 1000____
  { { 0b10010000, 0b10111111 }, TOO_LARGE },      // 11110100-1111____ 1001____-101_____
  { { 0b10000000, 0b10001111 }, TOO_LARGE_1000 }, // 11110101-1111____ 1000____
};

  simdjson_inline simd8<uint8_t> check_special_cases(const simd8<uint8_t> input, const simd8<uint8_t> prev1) {
    return BYTE_1[prev1] & BYTE_2_HIGH[input];
  }
  simdjson_inline simd8<uint8_t> check_multibyte_lengths(const simd8<uint8_t> input,
      const simd8<uint8_t> prev_input, const simd8<uint8_t> sc) {
    simd8<uint8_t> prev2 = input.prev<2>(prev_input);
    simd8<uint8_t> prev3 = input.prev<3>(prev_input);
    simd8<uint8_t> must23 = simd8<uint8_t>(must_be_2_3_continuation(prev2, prev3));
    simd8<uint8_t> must23_80 = must23 & uint8_t(0x80);
    return must23_80 ^ sc;
  }

  //
  // Return nonzero if there are incomplete multibyte characters at the end of the block:
  // e.g. if there is a 4-byte character, but it's 3 bytes from the end.
  //
  simdjson_inline simd8<uint8_t> is_incomplete(const simd8<uint8_t> input) {
    // If the previous input's last 3 bytes match this, they're too short (they ended at EOF):
    // ... 1111____ 111_____ 11______
#if SIMDJSON_IMPLEMENTATION_ICELAKE
    static const uint8_t max_array[64] = {
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 0xf0u-1, 0xe0u-1, 0xc0u-1
    };
#else
    static const uint8_t max_array[32] = {
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 0xf0u-1, 0xe0u-1, 0xc0u-1
    };
#endif
    const simd8<uint8_t> max_value(&max_array[sizeof(max_array)-sizeof(simd8<uint8_t>)]);
    return input.gt_bits(max_value);
  }

  struct utf8_checker {
    // If this is nonzero, there has been a UTF-8 error.
    simd8<uint8_t> error;
    // The last input we received
    simd8<uint8_t> prev_input_block;
    // Whether the last input we received was incomplete (used for ASCII fast path)
    simd8<uint8_t> prev_incomplete;

    //
    // Check whether the current bytes are valid UTF-8.
    //
    simdjson_inline void check_utf8_bytes(const simd8<uint8_t> input, const simd8<uint8_t> prev_input) {
      // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
      // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
      simd8<uint8_t> prev1 = input.prev<1>(prev_input);
      simd8<uint8_t> sc = check_special_cases(input, prev1);
      this->error |= check_multibyte_lengths(input, prev_input, sc);
    }

    // The only problem that can happen at EOF is that a multibyte character is too short
    // or a byte value too large in the last bytes: check_special_cases only checks for bytes
    // too large in the first of two bytes.
    simdjson_inline void check_eof() {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error |= this->prev_incomplete;
    }

    simdjson_inline void check_next_input(const simd8x64<uint8_t>& input) {
      if(simdjson_likely(is_ascii(input))) {
        this->error |= this->prev_incomplete;
      } else {
        // you might think that a for-loop would work, but under Visual Studio, it is not good enough.
        static_assert((simd8x64<uint8_t>::NUM_CHUNKS == 1)
                ||(simd8x64<uint8_t>::NUM_CHUNKS == 2)
                || (simd8x64<uint8_t>::NUM_CHUNKS == 4),
                "We support one, two or four chunks per 64-byte block.");
        SIMDJSON_IF_CONSTEXPR (simd8x64<uint8_t>::NUM_CHUNKS == 1) {
          this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
        } else SIMDJSON_IF_CONSTEXPR (simd8x64<uint8_t>::NUM_CHUNKS == 2) {
          this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
          this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
        } else SIMDJSON_IF_CONSTEXPR (simd8x64<uint8_t>::NUM_CHUNKS == 4) {
          this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
          this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
          this->check_utf8_bytes(input.chunks[2], input.chunks[1]);
          this->check_utf8_bytes(input.chunks[3], input.chunks[2]);
        }
        this->prev_incomplete = is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1]);
        this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1];
      }
    }
    // do not forget to call check_eof!
    simdjson_inline error_code errors() {
      return this->error.any_bits_set_anywhere() ? error_code::UTF8_ERROR : error_code::SUCCESS;
    }

  }; // struct utf8_checker
} // namespace utf8_validation

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_UTF8_LOOKUP4_ALGORITHM_H