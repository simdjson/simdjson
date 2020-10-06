namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace utf8_validation {

//
// Detect Unicode errors.
//
// UTF-8 is designed to allow multiple bytes and be compatible with ASCII. It's a fairly basic
// encoding that uses the first few bits on each byte to denote a "byte type", and all other bits
// are straight up concatenated into the final value. The first byte of a multibyte character is a
// "leading byte" and starts with N 1's, where N is the total number of bytes (110_____ = 2 byte
// lead). The remaining bytes of a multibyte character all start with 10. 1-byte characters just
// start with 0, because that's what ASCII looks like. Here's what each size looks like:
//
// - ASCII (7 bits):              0_______
// - 2 byte character (11 bits):  110_____ 10______
// - 3 byte character (17 bits):  1110____ 10______ 10______
// - 4 byte character (23 bits):  11110___ 10______ 10______ 10______
// - 5+ byte character (illegal): 11111___ <illegal>
//
// There are 5 classes of error that can happen in Unicode:
//
// - TOO_SHORT: when you have a multibyte character with too few bytes (i.e. missing continuation).
//   We detect this by looking for new characters (lead bytes) inside the range of a multibyte
//   character.
//
//   e.g. 11000000 01100001 (2-byte character where second byte is ASCII)
//
// - TOO_LONG: when there are more bytes in your character than you need (i.e. extra continuation).
//   We detect this by requiring that the next byte after your multibyte character be a new
//   character--so a continuation after your character is wrong.
//
//   e.g. 11011111 10111111 10111111 (2-byte character followed by *another* continuation byte)
//
// - TOO_LARGE: Unicode only goes up to U+10FFFF. These characters are too large.
//
//   e.g. 11110111 10111111 10111111 10111111 (bigger than 10FFFF).
//
// - OVERLONG: multibyte characters with a bunch of leading zeroes, where you could have
//   used fewer bytes to make the same character. Like encoding an ASCII character in 4 bytes is
//   technically possible, but UTF-8 disallows it so that there is only one way to write an "a".
//
//   e.g. 11000001 10100001 (2-byte encoding of "a", which only requires 1 byte: 01100001)
//
// - SURROGATE: Unicode U+D800-U+DFFF is a *surrogate* character, reserved for use in UCS-2 and
//   WTF-8 encodings for characters with > 2 bytes. These are illegal in pure UTF-8.
//
//   e.g. 11101101 10100000 10000000 (U+D800)
//
// - INVALID_5_BYTE: 5-byte, 6-byte, 7-byte and 8-byte characters are unsupported; Unicode does not
//   support values with more than 23 bits (which a 4-byte character supports).
//
//   e.g. 11111000 10100000 10000000 10000000 10000000 (U+800000)
//
// Legal utf-8 byte sequences per  http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94:
//
//   Code Points        1st       2s       3s       4s
//  U+0000..U+007F     00..7F
//  U+0080..U+07FF     C2..DF   80..BF
//  U+0800..U+0FFF     E0       A0..BF   80..BF
//  U+1000..U+CFFF     E1..EC   80..BF   80..BF
//  U+D000..U+D7FF     ED       80..9F   80..BF
//  U+E000..U+FFFF     EE..EF   80..BF   80..BF
//  U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
//  U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
//  U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
//
using namespace simd;

  // For a detailed description of the lookup2 algorithm, see the file HACKING.md under "UTF-8 validation (lookup2)".

  //
  // Find special case UTF-8 errors where the character is technically readable (has the right length)
  // but the *value* is disallowed.
  //
  // This includes overlong encodings, surrogates and values too large for Unicode.
  //
  // It turns out the bad character ranges can all be detected by looking at the first 12 bits of the
  // UTF-8 encoded character (i.e. all of byte 1, and the high 4 bits of byte 2). This algorithm does a
  // 3 4-bit table lookups, identifying which errors that 4 bits could match, and then &'s them together.
  // If all 3 lookups detect the same error, it's an error.
  //
  simdjson_really_inline simd8<uint8_t> check_special_cases(const simd8<uint8_t> input, const simd8<uint8_t> prev1) {
    //
    // These are the errors we're going to match for bytes 1-2, by looking at the first three
    // nibbles of the character: <high bits of byte 1>> & <low bits of byte 1> & <high bits of byte 2>
    //
    static const int OVERLONG_2  = 0x01; // 1100000_ 10______ (technically we match 10______ but we could match ________, they both yield errors either way)
    static const int OVERLONG_3  = 0x02; // 11100000 100_____ ________
    static const int OVERLONG_4  = 0x04; // 11110000 1000____ ________ ________
    static const int SURROGATE   = 0x08; // 11101101 [101_]____
    static const int TOO_LARGE   = 0x10; // 11110100 (1001|101_)____
    static const int TOO_LARGE_2 = 0x20; // 1111(1___|011_|0101) 10______

    // After processing the rest of byte 1 (the low bits), we're still not done--we have to check
    // byte 2 to be sure which things are errors and which aren't.
    // Since high_bits is byte 5, byte 2 is high_bits.prev<3>
    static const int CARRY = OVERLONG_2 | TOO_LARGE_2;
    const simd8<uint8_t> byte_2_high = input.shr<4>().lookup_16<uint8_t>(
        // ASCII: ________ [0___]____
        CARRY, CARRY, CARRY, CARRY,
        // ASCII: ________ [0___]____
        CARRY, CARRY, CARRY, CARRY,
        // Continuations: ________ [10__]____
        CARRY | OVERLONG_3 | OVERLONG_4, // ________ [1000]____
        CARRY | OVERLONG_3 | TOO_LARGE,  // ________ [1001]____
        CARRY | TOO_LARGE  | SURROGATE,  // ________ [1010]____
        CARRY | TOO_LARGE  | SURROGATE,  // ________ [1011]____
        // Multibyte Leads: ________ [11__]____
        CARRY, CARRY, CARRY, CARRY
    );

    const simd8<uint8_t> byte_1_high = prev1.shr<4>().lookup_16<uint8_t>(
      // [0___]____ (ASCII)
      0, 0, 0, 0,
      0, 0, 0, 0,
      // [10__]____ (continuation)
      0, 0, 0, 0,
      // [11__]____ (2+-byte leads)
      OVERLONG_2, 0,                       // [110_]____ (2-byte lead)
      OVERLONG_3 | SURROGATE,              // [1110]____ (3-byte lead)
      OVERLONG_4 | TOO_LARGE | TOO_LARGE_2 // [1111]____ (4+-byte lead)
    );

    const simd8<uint8_t> byte_1_low = (prev1 & 0x0F).lookup_16<uint8_t>(
      // ____[00__] ________
      OVERLONG_2 | OVERLONG_3 | OVERLONG_4, // ____[0000] ________
      OVERLONG_2,                           // ____[0001] ________
      0, 0,
      // ____[01__] ________
      TOO_LARGE,                            // ____[0100] ________
      TOO_LARGE_2,
      TOO_LARGE_2,
      TOO_LARGE_2,
      // ____[10__] ________
      TOO_LARGE_2, TOO_LARGE_2, TOO_LARGE_2, TOO_LARGE_2,
      // ____[11__] ________
      TOO_LARGE_2,
      TOO_LARGE_2 | SURROGATE,                            // ____[1101] ________
      TOO_LARGE_2, TOO_LARGE_2
    );

    return byte_1_high & byte_1_low & byte_2_high;
  }

  simdjson_really_inline simd8<uint8_t> check_multibyte_lengths(const simd8<uint8_t> input, const simd8<uint8_t> prev_input, const simd8<uint8_t> prev1) {
    simd8<uint8_t> prev2 = input.prev<2>(prev_input);
    simd8<uint8_t> prev3 = input.prev<3>(prev_input);

    // Cont is 10000000-101111111 (-65...-128)
    simd8<bool> is_continuation = simd8<int8_t>(input) < int8_t(-64);
    // must_be_continuation is architecture-specific because Intel doesn't have unsigned comparisons
    return simd8<uint8_t>(must_be_continuation(prev1, prev2, prev3) ^ is_continuation);
  }

  //
  // Return nonzero if there are incomplete multibyte characters at the end of the block:
  // e.g. if there is a 4-byte character, but it's 3 bytes from the end.
  //
  simdjson_really_inline simd8<uint8_t> is_incomplete(const simd8<uint8_t> input) {
    // If the previous input's last 3 bytes match this, they're too short (they ended at EOF):
    // ... 1111____ 111_____ 11______
    static const uint8_t max_array[32] = {
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 0b11110000u-1, 0b11100000u-1, 0b11000000u-1
    };
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
    simdjson_really_inline void check_utf8_bytes(const simd8<uint8_t> input, const simd8<uint8_t> prev_input) {
      // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
      // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
      simd8<uint8_t> prev1 = input.prev<1>(prev_input);
      this->error |= check_special_cases(input, prev1);
      this->error |= check_multibyte_lengths(input, prev_input, prev1);
    }

    // The only problem that can happen at EOF is that a multibyte character is too short.
    simdjson_really_inline void check_eof() {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error |= this->prev_incomplete;
    }

    simdjson_really_inline void check_next_input(const simd8x64<uint8_t>& input) {
      if (likely(is_ascii(input))) {
        // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
        // possibly finish them.
        this->error |= this->prev_incomplete;
      } else {
        this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
        for (int i=1; i<simd8x64<uint8_t>::NUM_CHUNKS; i++) {
          this->check_utf8_bytes(input.chunks[i], input.chunks[i-1]);
        }
        this->prev_incomplete = is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1]);
        this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1];
      }
    }

    simdjson_really_inline error_code errors() {
      return this->error.any_bits_set_anywhere() ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
    }

  }; // struct utf8_checker
} // namespace utf8_validation
} // unnamed namespace

using utf8_validation::utf8_checker;

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
} // namespace simdjson
