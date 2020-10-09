namespace simdjson {
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace utf8_validation {

//
// Detect Unicode errors.
//
// UTF-8 is designed to allow multiple bytes and be compatible with ASCII. It's a fairly basic
// encoding that uses the first few bits on each byte to denote a "byte type", and all other bits
// are straight up concatenated into the final value. The first byte of a multibyte character is a
// "leading byte" and starts with N 1's, where N is the total number of bytes (110_____ = 2 byte
// lead). The remaining bytes of a multibyte character all start with 10. 1-byte characters just
// start with 0, because that's what ASCII looks like. Here's what each size 
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

struct utf8_checker {
  // If this is nonzero, there has been a UTF-8 error.
  simd8<uint8_t> error;
  // The last input we received.
  simd8<uint8_t> prev_input_block;
  // If there were leads at the end of the previous block, to be continued in the next.
  simd8<uint8_t> prev_incomplete;

  //
  // These are the bits in lead_flags. Its main purpose is to tell you what kind of lead character
  // it is (1,2,3 or 4--or none if it's continuation), but it also maps 4 other bytes that will be
  // used to detect other kinds of errors.
  //
  // LEAD_4 is first because we use a << trick in get_byte_3_4_5_errors to turn LEAD_2 -> LEAD_3,
  // LEAD_3 -> LEAD_4, and we want LEAD_4 to turn into nothing since there is no LEAD_5. This trick
  // lets us use one constant table instead of 3, possibly saving registers on systems with fewer
  // registers.
  //
  static const uint8_t LEAD_4      = 0x01; // [1111]____ 10______ 10______ 10______ (0_|11)__
  static const uint8_t LEAD_3      = 0x02; // [1110]____ 10______ 10______ (0|11)__
  static const uint8_t LEAD_2      = 0x04; // [110_]____ 10______ (0|11)__
  static const uint8_t LEAD_1      = 0x08; // [0___]____ (0|11)__
  static const uint8_t LEAD_2_PLUS = 0x10; // [11__]____ ...
  static const uint8_t LEAD_1100   = 0x20; // [1100]____ ...
  static const uint8_t LEAD_1110   = 0x40; // [1110]____ ...
  static const uint8_t LEAD_1111   = 0x80; // [1111]____ ...

  // Prepare fast_path_error in case the next block is ASCII
  simdjson_really_inline void set_fast_path_error() {
    // If any of the last 3 bytes in the input needs a continuation at the start of the next input,
    // it is an error for the next input to be ASCII.
    // static const uint8_t incomplete_long[32] = {
    //   0, 0, 0, 0, 0, 0, 0, 0,
    //   0, 0, 0, 0, 0, 0, 0, 0,
    //   0, 0, 0, 0, 0, 0, 0, 0,
    //   0, 0, 0, 0, 0, LEAD_4, LEAD_4 | LEAD_3, LEAD_4 | LEAD_3 | LEAD_2
    // };
    // const simd8<uint8_t> incomplete(&incomplete_long[sizeof(incomplete_long) - sizeof(simd8<uint8_t>)]);
    // this->prev_incomplete = lead_flags & incomplete;
    // If the previous input's last 3 bytes match this, they're too short (they ended at EOF):
    // ... 1111____ 111_____ 11______
    static const uint8_t last_len[32] = {
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0b11110000u-1, 0b11100000u-1, 0b11000000u-1
    };
    const simd8<uint8_t> max_value(&last_len[sizeof(last_len)-sizeof(simd8<uint8_t>)]);
    // If anything is > the desired value, there will be a nonzero value in the result.
    this->prev_incomplete = this->prev_input_block.saturating_sub(max_value);
  }

  simdjson_really_inline simd8<uint8_t> get_lead_flags(const simd8<uint8_t> high_bits, const simd8<uint8_t> prev_high_bits) {
    // Total: 2 instructions, 1 constant
    // - 1 byte shift (shuffle)
    // - 1 table lookup (shuffle)
    // - 1 table constant

    // high_bits is byte 5, so lead is high_bits.prev<4>()
    return high_bits.prev<4>(prev_high_bits).lookup_16<uint8_t>(
      LEAD_1, LEAD_1, LEAD_1, LEAD_1,   // [0___]____ (ASCII)
      LEAD_1, LEAD_1, LEAD_1, LEAD_1,   // [0___]____ (ASCII)
      0,      0,      0,      0,        // [10__]____ (continuation)
      LEAD_2 | LEAD_2_PLUS | LEAD_1100, // [1100]____
      LEAD_2 | LEAD_2_PLUS,             // [110_]____
      LEAD_3 | LEAD_2_PLUS | LEAD_1110, // [1110]____
      LEAD_4 | LEAD_2_PLUS | LEAD_1111  // [1111]____
    );
  }

  // Find errors in bytes 1 and 2 together (one single multi-nibble &)
  simdjson_really_inline simd8<uint8_t> get_byte_1_2_errors(const simd8<uint8_t> input, const simd8<uint8_t> prev_input, const simd8<uint8_t> high_bits, const simd8<uint8_t> prev_high_bits) {
    //
    // These are the errors we're going to match for bytes 1-2, by looking at the first three
    // nibbles of the character: lead_flags & <low bits of byte 1> & <high bits of byte 2>
    //
    // The important thing here is that these constants all take up *different* bits, since they
    // match different patterns. This is why there are 2 LEAD_4 and 2 LEAD_3s in lead_flags, among
    // other things.
    //
    static const int TOO_SHORT_2 = LEAD_2_PLUS; // 11______ (0___|11__)____
    static const int TOO_LONG_1  = LEAD_1;      // 0_______ 10______
    static const int OVERLONG_2  = LEAD_1100;   // 1100000_ ________ (technically we match 10______ but we could match ________, they both yield errors either way)
    static const int OVERLONG_3  = LEAD_3;      // 11100000 100_____ ________
    static const int OVERLONG_4  = LEAD_4;      // 11110000 1000____ ________ ________
    static const int TOO_LARGE   = LEAD_1111;   // 11110100 (1001|101_)____
    static const int SURROGATE   = LEAD_1110;   // 11101101 [101_]____

    // Total: 4 instructions, 2 constants
    // - 2 table lookups (shuffles)
    // - 1 byte shift (shuffle)
    // - 1 "and"
    // - 2 table constants

    // After processing the rest of byte 1 (the low bits), we're still not done--we have to check
    // byte 2 to be sure which things are errors and which aren't.
    // Since input is byte 5, byte 1 is input.prev<4>
    const simd8<uint8_t> byte_1_flags = (input.prev<4>(prev_input) & 0x0F).lookup_16<uint8_t>(
      // ____[00__] ________
      TOO_SHORT_2 | TOO_LONG_1 | OVERLONG_2 | OVERLONG_3 | OVERLONG_4, // ____[0000] ________
      TOO_SHORT_2 | TOO_LONG_1 | OVERLONG_2,                           // ____[0001] ________
      TOO_SHORT_2 | TOO_LONG_1, TOO_SHORT_2 | TOO_LONG_1,
      // ____[01__] ________
      TOO_SHORT_2 | TOO_LONG_1 | TOO_LARGE,                            // ____[0100] ________
      TOO_SHORT_2 | TOO_LONG_1, TOO_SHORT_2 | TOO_LONG_1, TOO_SHORT_2 | TOO_LONG_1,
      // ____[10__] ________
      TOO_SHORT_2 | TOO_LONG_1, TOO_SHORT_2 | TOO_LONG_1, TOO_SHORT_2 | TOO_LONG_1, TOO_SHORT_2 | TOO_LONG_1,
      // ____[11__] ________
      TOO_SHORT_2 | TOO_LONG_1,
      TOO_SHORT_2 | TOO_LONG_1 | SURROGATE,                            // ____[1101] ________
      TOO_SHORT_2 | TOO_LONG_1, TOO_SHORT_2 | TOO_LONG_1
    );
    // Since high_bits is byte 5, byte 2 is high_bits.prev<3>
    const simd8<uint8_t> byte_2_flags = high_bits.prev<3>(prev_high_bits).lookup_16<uint8_t>(
        // ASCII: ________ [0___]____
        OVERLONG_2 | TOO_SHORT_2, OVERLONG_2 | TOO_SHORT_2, OVERLONG_2 | TOO_SHORT_2, OVERLONG_2 | TOO_SHORT_2,
        // ASCII: ________ [0___]____
        OVERLONG_2 | TOO_SHORT_2, OVERLONG_2 | TOO_SHORT_2, OVERLONG_2 | TOO_SHORT_2, OVERLONG_2 | TOO_SHORT_2,
        // Continuations: ________ [10__]____
        OVERLONG_2 | TOO_LONG_1 | OVERLONG_3 | OVERLONG_4, // ________ [1000]____
        OVERLONG_2 | TOO_LONG_1 | OVERLONG_3 | SURROGATE,  // ________ [1001]____
        OVERLONG_2 | TOO_LONG_1 | TOO_LARGE  | SURROGATE,  // ________ [1010]____
        OVERLONG_2 | TOO_LONG_1 | TOO_LARGE  | SURROGATE,  // ________ [1011]____
        // Multibyte Leads: ________ [11__]____
        OVERLONG_2 | TOO_SHORT_2, OVERLONG_2 | TOO_SHORT_2, OVERLONG_2 | TOO_SHORT_2, OVERLONG_2 | TOO_SHORT_2
    );
    return byte_1_flags & byte_2_flags;
  }

  simdjson_really_inline simd8<uint8_t> get_byte_3_4_5_errors(const simd8<uint8_t> high_bits, const simd8<uint8_t> prev_high_bits) {
    // Total 7 instructions, 3 simd constants:
    // - 3 table lookups (shuffles)
    // - 2 byte shifts (shuffles)
    // - 2 "or"
    // - 1 table constant

    const simd8<uint8_t> byte_3_table = simd8<uint8_t>::repeat_16(
        // TOO_SHORT ASCII:           111_____ ________ [0___]____
        LEAD_3, LEAD_3, LEAD_3, LEAD_3,
        LEAD_3, LEAD_3, LEAD_3, LEAD_3,
        // TOO_LONG  Continuations:   110_____ ________ [10__]____
        LEAD_2, LEAD_2, LEAD_2, LEAD_2,
        // TOO_SHORT Multibyte Leads: 111_____ ________ [11__]____
        LEAD_3, LEAD_3, LEAD_3, LEAD_3
    );
    const simd8<uint8_t> byte_4_table = byte_3_table.shr<1>(); // TOO_SHORT: LEAD_4, TOO_LONG: LEAD_3
    const simd8<uint8_t> byte_5_table = byte_3_table.shr<2>(); // TOO_SHORT: <none>, TOO_LONG: LEAD_4

    // high_bits is byte 5, high_bits.prev<2> is byte 3 and high_bits.prev<1> is byte 4
    return high_bits.prev<2>(prev_high_bits).lookup_16(byte_3_table) |
           high_bits.prev<1>(prev_high_bits).lookup_16(byte_4_table) |
           high_bits.lookup_16(byte_5_table);
  }

  // Check whether the current bytes are valid UTF-8.
  // At the end of the function, previous gets updated
  // This should come down to 22 instructions if table definitions are in registers--30 if not.
  simdjson_really_inline simd8<uint8_t> check_utf8_bytes(const simd8<uint8_t> input, const simd8<uint8_t> prev_input) {
    // When we process bytes M through N, we look for lead characters in M-4 through N-4. This allows
    // us to look for all errors related to any lead character at one time (since UTF-8 characters
    // can only be up to 4 bytes, and the next byte after a character finishes must be another lead,
    // we never need to look more than 4 bytes past the current one to fully validate).
    // This way, we have all relevant bytes around and can save ourselves a little overflow and
    // several instructions on each loop.

    // Total: 22 instructions, 7 simd constants
    // Local: 8 instructions, 1 simd constant
    // - 2 bit shifts
    // - 1 byte shift (shuffle)
    // - 3 "or"
    // - 1 "and"
    // - 1 saturating_sub
    // - 1 constant (0b11111000-1)
    // lead_flags: 2 instructions, 1 simd constant
    // - 1 byte shift (shuffle)
    // - 1 table lookup (shuffle)
    // - 1 table constant
    // byte_1_2_errors: 5 instructions, 2 simd constants
    // - 2 table lookups (shuffles)
    // - 2 byte shifts (shuffles)
    // - 1 "and"
    // - 2 table constants
    // byte_3_4_5_errors: 7 instructions, 3 simd constants
    // - 3 table lookups (shuffles)
    // - 2 byte shifts (shuffles)
    // - 2 "or"
    // - 3 table constants

    const simd8<uint8_t> high_bits = input.shr<4>();
    const simd8<uint8_t> prev_high_bits = prev_input.shr<4>();
    const simd8<uint8_t> lead_flags = get_lead_flags(high_bits, prev_high_bits);
    const simd8<uint8_t> byte_1_2_errors = get_byte_1_2_errors(input, prev_input, high_bits, prev_high_bits);
    const simd8<uint8_t> byte_3_4_5_errors = get_byte_3_4_5_errors(high_bits, prev_high_bits);
    // Detect illegal 5-byte+ Unicode values. We can't do this as part of byte_1_2_errors  because
    // it would need a third lead_flag = 1111, and we've already used up all 8 between
    // byte_1_2_errors and byte_3_4_5_errors.
    const simd8<uint8_t> too_large = input.saturating_sub(0b11111000-1); // too-large values will be nonzero
    return too_large | (lead_flags & (byte_1_2_errors | byte_3_4_5_errors));
  }

  // TODO special case start of file, too, so that small documents are efficient! No shifting needed ...

  // The only problem that can happen at EOF is that a multibyte character is too short.
  simdjson_really_inline void check_eof() {
    // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
    // possibly finish them.
    this->error |= this->prev_incomplete;
  }

  simdjson_really_inline void check_next_input(const simd8x64<uint8_t>& input) {
    simd8<uint8_t> bits = input.reduce_or();
    if (simdjson_likely(!bits.any_bits_set_anywhere(0b10000000u))) {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error |= this->prev_incomplete;
    } else {
      this->error |= this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
      for (int i=1; i<simd8x64<uint8_t>::NUM_CHUNKS; i++) {
        this->error |= this->check_utf8_bytes(input.chunks[i], input.chunks[i-1]);
      }
      this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1];
      this->set_fast_path_error();
    }
  }

  simdjson_really_inline error_code errors() {
    return this->error.any_bits_set_anywhere() ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
  }

}; // struct utf8_checker

} // namespace utf8_validation
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
