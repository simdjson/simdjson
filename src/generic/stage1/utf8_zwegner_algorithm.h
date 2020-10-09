namespace simdjson {
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
//
// Detect UTF-8 errors.
//
// Copied and adapted from algorithm by @zwegner: https://github.com/zwegner/faster-utf8-validator
//
// UTF-8 Refresher
// ---------------
//
// UTF-8 is designed to allow multiple bytes and be compatible with ASCII. It's a fairly basic
// encoding that uses the first few bits on each byte to denote a "byte type", and all other bits
// are straight up concatenated into the final value. The first byte of a multibyte character is a
// "leading byte" and starts with N 1's, where N is the total number of bytes (110_____ = 2 byte
// lead). The remaining bytes of a multibyte character all start with 10. 1-byte characters just
// start with 0, because that's what ASCII looks like. Here's what each size 
//
// | Character Length            | UTF-8 Byte Sequence                   |
// |-----------------------------|---------------------------------------|
// | ASCII (7 bits):             | `0_______`                            |
// | 2 byte character (11 bits)  | `110_____ 10______`                   |
// | 3 byte character (17 bits)  | `1110____ 10______ 10______`          |
// | 4 byte character (23 bits)  | `11110___ 10______ 10______ 10______` |
// | 5+ byte character (illegal) | `11111___` <illegal>                  |
//
// UTF-8 Error Classes
// -------------------
//
// There are 5 classes of error that can happen in UTF-8:
//
// ### Too short (missing continuations)
//
// TOO_SHORT: when you have a multibyte character with too few bytes (i.e. missing continuation).
// We detect this by looking for new characters (lead bytes) inside the range of a multibyte
// character.
//
// e.g. `11000000 01100001` (2-byte character where second byte is ASCII)
//
// ### Too long (stray continuations)
//
// TOO_LONG: when there are more bytes in your character than you need (i.e. extra continuation).
// We detect this by requiring that the next byte after your multibyte character be a new
// character--so a continuation after your character is wrong.
//
// e.g. `11011111 10111111 10111111` (2-byte character followed by *another* continuation byte)
//
// ### Too large (out of range for unicode)
//
// TOO_LARGE: Unicode only goes up to U+10FFFF. These characters are too large.
//
// e.g. `11110111 10111111 10111111 10111111` (bigger than 10FFFF).
//
// ### Overlong encoding (used more bytes than needed)
//
// Multibyte characters with a bunch of leading zeroes, where you could have
// used fewer bytes to make the same character, are considered *overlong encodings*. They are
// disallowed in UTF-8 to ensure there is only one way to write a single Unicode codepoint, making strings
// easier to search. Like encoding an ASCII character in 2 bytes is technically possible, but UTF-8
// disallows it so that you only have to search for the ASCII character `a` to find it.
//
// e.g. `11000001 10100001` (2-byte encoding of "a", which only requires 1 byte: 01100001)
//
// ### Surrogate characters
//
// Unicode U+D800-U+DFFF is a *surrogate* character, reserved for use in UCS-2 and WTF-8 encodings
// for characters with > 2 bytes. These are illegal in pure UTF-8.
//
// e.g. `11101101 10100000 10000000` (U+D800)
//
// ### 5+ byte characters
// 
// INVALID_5_BYTE: 5-byte, 6-byte, 7-byte and 8-byte characters are unsupported; Unicode does not
// support values with more than 23 bits (which a 4-byte character supports).
//
// Even if these were supported, anything with 5 bytes would be either too large (bigger than the
// Unicode max value), or overlong (could fit in 4+ bytes).
//
// e.g. `11111000 10100000 10000000 10000000 10000000` (U+800000)
//   
// Legal utf-8 byte sequences per  http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94:
// 
//  |  Code Points       |  1st   |  2nd   |   3s   |   4s   |
//  |--------------------|--------|--------|--------|--------|
//  | U+0000..U+007F     | 00..7F |        |        |        |
//  | U+0080..U+07FF     | C2..DF | 80..BF |        |        |
//  | U+0800..U+0FFF     | E0     | A0..BF | 80..BF |        |
//  | U+1000..U+CFFF     | E1..EC | 80..BF | 80..BF |        |
//  | U+D000..U+D7FF     | ED     | 80..9F | 80..BF |        |
//  | U+E000..U+FFFF     | EE..EF | 80..BF | 80..BF |        |
//  | U+10000..U+3FFFF   | F0     | 90..BF | 80..BF | 80..BF |
//  | U+40000..U+FFFFF   | F1..F3 | 80..BF | 80..BF | 80..BF |
//  | U+100000..U+10FFFF | F4     | 80..8F | 80..BF | 80..BF |
//
// Algorithm
// ---------
//
// This validator works in two basic steps: checking continuation bytes, and
// handling special cases. Each step works on one vector's worth of input
// bytes at a time.
//
using namespace simd;

using vmask_t = simd8<bool>::bitmask_t;
using vmask2_t = simd8<bool>::bitmask2_t;

struct utf8_checker {
  simd8<uint8_t> special_case_errors;
  simd8<uint8_t> prev_bytes;
  vmask2_t last_cont;
  vmask_t length_errors;

  //
  // Check for missing / extra continuation bytes.
  //
  // The continuation bytes are handled in a fairly straightforward manner in
  // the scalar domain. A mask is created from the input byte vector for each
  // of the highest four bits of every byte. The first mask allows us to quickly
  // skip pure ASCII input vectors, which have no bits set. The first and
  // (inverted) second masks together give us every continuation byte (10xxxxxx).
  // The other masks are used to find prefixes of multi-byte code points (110,
  // 1110, 11110). For these, we keep a "required continuation" mask, by shifting
  // these masks 1, 2, and 3 bits respectively forward in the byte stream. That
  // is, we take a mask of all bytes that start with 11, and shift it left one
  // bit forward to get the mask of all the first continuation bytes, then do the
  // same for the second and third continuation bytes. Here's an example input
  // sequence along with the corresponding masks:
  //
  //   bytes:        61 C3 80 62 E0 A0 80 63 F0 90 80 80 00
  //   code points:  61|C3 80|62|E0 A0 80|63|F0 90 80 80|00
  //   # of bytes:   1 |2  - |1 |3  -  - |1 |4  -  -  - |1
  //   cont. mask 1: -  -  1  -  -  1  -  -  -  1  -  -  -
  //   cont. mask 2: -  -  -  -  -  -  1  -  -  -  1  -  -
  //   cont. mask 3: -  -  -  -  -  -  -  -  -  -  -  1  -
  //   cont. mask *: 0  0  1  0  0  1  1  0  0  1  1  1  0
  //
  // The final required continuation mask is then compared with the mask of
  // actual continuation bytes, and must match exactly in valid UTF-8. The only
  // complication in this step is that the shifted masks can cross vector
  // boundaries, so we need to keep a "carry" mask of the bits that were shifted
  // past the boundary in the last loop iteration.
  //
  simdjson_really_inline void check_length_errors(const simd8<uint8_t> bytes, const vmask_t bit_7) {
    // Compute the continuation byte mask by finding bytes that start with
    // 11x, 111x, and 1111. For each of these prefixes, we get a bitmask
    // and shift it forward by 1, 2, or 3. This loop should be unrolled by
    // the compiler, and the (n == 1) branch inside eliminated.
    //
    // NOTE (@jkeiser): I unrolled the for(i=1..3) loop because I don't trust compiler unrolling
    // anymore. This should be exactly equivalent and yield the same optimizations (and also lets
    // us rearrange statements if we so desire).

    // We add the shifted mask here instead of ORing it, which would
    // be the more natural operation, so that this line can be done
    // with one lea. While adding could give a different result due
    // to carries, this will only happen for invalid UTF-8 sequences,
    // and in a way that won't cause it to pass validation. Reasoning:
    // Any bits for required continuation bytes come after the bits
    // for their leader bytes, and are all contiguous. For a carry to
    // happen, two of these bit sequences would have to overlap. If
    // this is the case, there is a leader byte before the second set
    // of required continuation bytes (and thus before the bit that
    // will be cleared by a carry). This leader byte will not be
    // in the continuation mask, despite being required. QEDish.
    // Which bytes are required to be continuation bytes
    vmask2_t cont_required = this->last_cont;

    // 2-byte lead: 11______
    const vmask_t bit_6 = bytes.get_bit<6>();
    const vmask_t lead_2_plus = bit_7 & bit_6;       // 11______
    cont_required += vmask2_t(lead_2_plus) << 1;

    // 3-byte lead: 111_____
    const vmask_t bit_5 = bytes.get_bit<5>();
    const vmask_t lead_3_plus = lead_2_plus & bit_5; // 111_____
    cont_required += vmask2_t(lead_3_plus) << 2;

    // 4-byte lead: 1111____
    const vmask_t bit_4 = bytes.get_bit<4>();
    const vmask_t lead_4_plus = lead_3_plus & bit_4;
    cont_required += vmask2_t(lead_4_plus) << 3;

    const vmask_t cont = bit_7 ^ lead_2_plus;        // 10______ TODO &~ bit_6 might be fine, and involve less data dependency

    // Check that continuation bytes match. We must cast req from vmask2_t
    // (which holds the carry mask in the upper half) to vmask_t, which
    // zeroes out the upper bits
    //
    // NOTE (@jkeiser): I turned the if() statement here into this->has_error for performance in
    // success cases: instead of spending time testing the result and introducing a branch (which
    // can affect performance even if it's easily predictable), we test once at the end.
    // The ^ is equivalent to !=, however, leaving a 1 where the bits are different and 0 where they
    // are the same.
    this->length_errors |= cont ^ vmask_t(cont_required);

    this->last_cont = cont_required >> sizeof(simd8<uint8_t>);
  }

  //
  // These constants define the set of error flags in check_special_cases().
  //
  static const uint8_t OVERLONG_2  = 0x01; // 1100000_         ________         Could have been encoded in 1 byte
  static const uint8_t OVERLONG_3  = 0x02; // 11100000         100_____         Could have been encoded in 2 bytes
  static const uint8_t SURROGATE   = 0x04; // 11101010         101_____         Surrogate pairs
  static const uint8_t TOO_LARGE   = 0x08; // 11110100         (1001|101_)____ > U+10FFFF
  static const uint8_t TOO_LARGE_2 = 0x10; // 1111(0101..1111) ________       > U+10FFFF
  static const uint8_t OVERLONG_4  = 0x20; // 11110000         1000____         Could have been encoded in 3 bytes

  //
  // Check for special-case errors with table lookups on the first 3 nibbles (first 2 bytes).
  //
  // Besides the basic prefix coding of UTF-8, there are several invalid byte
  // sequences that need special handling. These are due to three factors:
  // code points that could be described in fewer bytes, code points that are
  // part of a surrogate pair (which are only valid in UTF-16), and code points
  // that are past the highest valid code point U+10FFFF.
  //
  // All of the invalid sequences can be detected by independently observing
  // the first three nibbles of each code point. Since AVX2 can do a 4-bit/16-byte
  // lookup in parallel for all 32 bytes in a vector, we can create bit masks
  // for all of these error conditions, look up the bit masks for the three
  // nibbles for all input bytes, and AND them together to get a final error mask,
  // that must be all zero for valid UTF-8. This is somewhat complicated by
  // needing to shift the error masks from the first and second nibbles forward in
  // the byte stream to line up with the third nibble.
  //
  // We have these possible values for valid UTF-8 sequences, broken down
  // by the first three nibbles:
  //
  //   1st   2nd   3rd   comment
  //   0..7  0..F        ASCII
  //   8..B  0..F        continuation bytes
  //   C     2..F  8..B  C0 xx and C1 xx can be encoded in 1 byte
  //   D     0..F  8..B  D0..DF are valid with a continuation byte
  //   E     0     A..B  E0 8x and E0 9x can be encoded with 2 bytes
  //         1..C  8..B  E1..EC are valid with continuation bytes
  //         D     8..9  ED Ax and ED Bx correspond to surrogate pairs
  //         E..F  8..B  EE..EF are valid with continuation bytes
  //   F     0     9..B  F0 8x can be encoded with 3 bytes
  //         1..3  8..B  F1..F3 are valid with continuation bytes
  //         4     8     F4 8F BF BF is the maximum valid code point
  //
  // That leaves us with these invalid sequences, which would otherwise fit
  // into UTF-8's prefix encoding. Each of these invalid sequences needs to
  // be detected separately, with their own bits in the error mask.
  //
  //   1st   2nd   3rd   error bit
  //   C     0..1  0..F  0x01
  //   E     0     8..9  0x02
  //         D     A..B  0x04
  //   F     0     0..8  0x08
  //         4     9..F  0x10
  //         5..F  0..F  0x20
  //
  // For every possible value of the first, second, and third nibbles, we keep
  // a lookup table that contains the bitwise OR of all errors that that nibble
  // value can cause. For example, the first nibble has zeroes in every entry
  // except for C, E, and F, and the third nibble lookup has the 0x21 bits in
  // every entry, since those errors don't depend on the third nibble. After
  // doing a parallel lookup of the first/second/third nibble values for all
  // bytes, we AND them together. Only when all three have an error bit in common
  // do we fail validation.
  //
  simdjson_really_inline void check_special_cases(const simd8<uint8_t> bytes) {
    const simd8<uint8_t> shifted_bytes = bytes.prev<1>(this->prev_bytes);
    this->prev_bytes = bytes;

    // Look up error masks for three consecutive nibbles. We need to
    // AND with 0x0F for each one, because vpshufb has the neat
    // "feature" that negative values in an index byte will result in 
    // a zero.
    simd8<uint8_t> nibble_1_error = shifted_bytes.shr<4>().lookup_16<uint8_t>(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,

        OVERLONG_2,                          // [1100]000_         ________        Could have been encoded in 1 byte
        0,
        OVERLONG_3 | SURROGATE,              // [1110]0000         100_____        Could have been encoded in 2 bytes
                                             // [1110]1010         101_____        Surrogate pairs
        OVERLONG_4 | TOO_LARGE | TOO_LARGE_2 // [1111]0000         1000____        Could have been encoded in 3 bytes
                                             // [1111]0100         (1001|101_)____ > U+10FFFF
    );

    simd8<uint8_t> nibble_2_error = (shifted_bytes & 0x0F).lookup_16<uint8_t>(
      OVERLONG_2 | OVERLONG_3 | OVERLONG_4,  // 1100[000_]       ________        Could have been encoded in 1 byte
                                             // 1110[0000]       100_____        Could have been encoded in 2 bytes
                                             // 1111[0000]       1000____        Could have been encoded in 3 bytes
      OVERLONG_2,
      0,
      0,

      TOO_LARGE,                             // 1111[0100]       (1001|101_)____ > U+10FFFF
      TOO_LARGE_2,                           // 1111[0101..1111] ________        > U+10FFFF
      TOO_LARGE_2,
      TOO_LARGE_2,
      
      TOO_LARGE_2,
      TOO_LARGE_2,
      TOO_LARGE_2,
      TOO_LARGE_2,

      TOO_LARGE_2,
      TOO_LARGE_2 | SURROGATE,               // 1110[1010]       101_____        Surrogate pairs
      TOO_LARGE_2, TOO_LARGE_2
    );

    // Errors that apply no matter what the third byte is
    const uint8_t CARRY = OVERLONG_2 | TOO_LARGE_2; // 1100000_         [____]____        Could have been encoded in 1 byte
                                                    // 1111(0101..1111) [____]____        > U+10FFFF
    simd8<uint8_t> nibble_3_error = bytes.shr<4>().lookup_16<uint8_t>(
      CARRY, CARRY, CARRY, CARRY,

      CARRY, CARRY, CARRY, CARRY,

      CARRY | OVERLONG_3 | OVERLONG_4,        // 11100000       [100_]____       Could have been encoded in 2 bytes
                                              // 11110000       [1000]____       Could have been encoded in 3 bytes
      CARRY | OVERLONG_3 | TOO_LARGE,         // 11100000       [100_]____       Could have been encoded in 2 bytes
                                              // 11110100       [1001|101_]____  > U+10FFFF
      CARRY | SURROGATE | TOO_LARGE,          // 11101010       [101_]____       Surrogate pairs
      CARRY | SURROGATE | TOO_LARGE,

      CARRY, CARRY, CARRY, CARRY
    );

    // Check if any bits are set in all three error masks
    //
    // NOTE (@jkeiser): I turned the if() statement here into this->has_error for performance in
    // success cases: instead of spending time testing the result and introducing a branch (which
    // can affect performance even if it's easily predictable), we test once at the end.
    this->special_case_errors |= nibble_1_error & nibble_2_error & nibble_3_error;
  }

  // check whether the current bytes are valid UTF-8
  // at the end of the function, previous gets updated
  simdjson_really_inline void check_utf8_bytes(const simd8<uint8_t> bytes, const vmask_t bit_7) {
    this->check_length_errors(bytes, bit_7);
    this->check_special_cases(bytes);
  }

  simdjson_really_inline void check_next_input(const simd8<uint8_t> bytes) {
    vmask_t bit_7 = bytes.get_bit<7>();
    if (simdjson_unlikely(bit_7)) {
      // TODO (@jkeiser): To work with simdjson's caller model, I moved the calculation of
      // shifted_bytes inside check_utf8_bytes. I believe this adds an extra instruction to the hot
      // path (saving prev_bytes), which is undesirable, though 2 register accesses vs. 1 memory
      // access might be a wash. Come back and try the other way.
      this->check_utf8_bytes(bytes, bit_7);
    } else {
      this->length_errors |= this->last_cont;
    }
  }

  simdjson_really_inline void check_next_input(const simd8x64<uint8_t>& in) {
    for (int i=0; i<simd8x64<uint8_t>::NUM_CHUNKS; i++) {
      this->check_next_input(in.chunks[i]);
    }
  }

  simdjson_really_inline error_code errors() {
    return (this->special_case_errors.any_bits_set_anywhere() | this->length_errors) ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
  }
}; // struct utf8_checker

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
