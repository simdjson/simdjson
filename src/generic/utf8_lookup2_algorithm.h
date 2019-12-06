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
  // If there were leads at the end of the previous block, to be continued in the next.
  simd8<uint8_t> prev_incomplete;

  static const uint8_t HIGH_BIT = 0b10000000u;

  // Prepare fast_path_error in case the next block is ASCII
  really_inline simd8<uint8_t> is_incomplete(const simd8<uint8_t> input) {
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
    return input.saturating_sub(max_value);
  }

  //
  // Validate the length of multibyte characters (that each multibyte character has the right number
  // of continuation characters, and that all continuation characters are part of a multibyte
  // character).
  //
  // Algorithm
  // =========
  //
  // This algorithm compares *expected* continuation characters with *actual* continuation bytes,
  // and emits an error anytime there is a mismatch.
  //
  // For example, in the string "𝄞₿֏ab", which has a 4-, 3-, 2- and 1-byte
  // characters, the file will look like this:
  //
  // | Character             | 𝄞                 | ₿            | ֏       | a  | b  |
  // |-----------------------|----|----|----|----|----|----|----|----|----|----|----|
  // | Character Length      |  4                |  3           |  2      |  1 |  1 |
  // | Byte                  | F0 | 9D | 84 | 9E | E2 | 82 | BF | D6 | 8F | 61 | 62 |
  // | is_second_byte        |    |  X |    |    |    |  X |    |    |  X |    |    |
  // | is_third_byte         |    |    |  X |    |    |    |  X |    |    |    |    |
  // | is_fourth_byte        |    |    |    |  X |    |    |    |    |    |    |    |
  // | expected_continuation |    |  X |  X |  X |    |  X |  X |    |  X |    |    |
  // | is_continuation       |    |  X |  X |  X |    |  X |  X |    |  X |    |    |
  //
  // The errors here are basically (Second Byte OR Third Byte OR Fourth Byte == Continuation):
  //
  // - **Extra Continuations:** Any continuation that is not a second, third or fourth byte is not
  //   part of a valid 2-, 3- or 4-byte character and is thus an error. It could be that it's just
  //   floating around extra outside of any character, or that there is an illegal 5-byte character,
  //   or maybe it's at the beginning of the file before any characters have started; but it's an
  //   error in all these cases.
  // - **Missing Continuations:** Any second, third or fourth byte that *isn't* a continuation is an error, because that means
  //   we started a new character before we were finished with the current one.
  //
  // Getting the Previous Bytes
  // --------------------------
  //
  // Because we want to know if a byte is the *second* (or third, or fourth) byte of a multibyte
  // character, we need to "shift the bytes" to find that out. This is what they mean:
  //
  // - `is_continuation`: if the current byte is a continuation.
  // - `is_second_byte`: if 1 byte back is the start of a 2-, 3- or 4-byte character.
  // - `is_third_byte`: if 2 bytes back is the start of a 3- or 4-byte character.
  // - `is_fourth_byte`: if 3 bytes back is the start of a 4-byte character.
  //
  // We use shuffles to go n bytes back, selecting part of the current `input` and part of the
  // `prev_input` (search for `.prev<1>`, `.prev<2>`, etc.). These are passed in by the caller
  // function, because the 1-byte-back data is used by other checks as well.
  //
  // Getting the Continuation Mask
  // -----------------------------
  //
  // Once we have the right bytes, we have to get the masks. To do this, we treat UTF-8 bytes as
  // numbers, using signed `<` and `>` operations to check if they are continuations or leads.
  // In fact, we treat the numbers as *signed*, partly because it helps us, and partly because
  // Intel's SIMD presently only offers signed `<` and `>` operations (not unsigned ones).
  //
  // In UTF-8, bytes that start with the bits 110, 1110 and 11110 are 2-, 3- and 4-byte "leads,"
  // respectively, meaning they expect to have 1, 2 and 3 "continuation bytes" after them.
  // Continuation bytes start with 10, and ASCII (1-byte characters) starts with 0.
  //
  // When treated as signed numbers, they look like this:
  //
  // | Type         | High Bits  | Binary Range | Signed |
  // |--------------|------------|--------------|--------|
  // | ASCII        | `0`        | `01111111`   |   127  |
  // |              |            | `00000000`   |     0  |
  // |--------------|------------|--------------|--------|
  // | 4+-Byte Lead | `1111`     | `11111111`   |    -1  |
  // |              |            | `11110000    |   -16  |
  // |--------------|------------|--------------|--------|
  // | 3-Byte Lead  | `1110`     | `11101111`   |   -17  |
  // |              |            | `11100000    |   -32  |
  // |--------------|------------|--------------|--------|
  // | 2-Byte Lead  | `110`      | `11011111`   |   -33  |
  // |              |            | `11000000    |   -64  |
  // |--------------|------------|--------------|--------|
  // | Continuation | `10`       | `10111111`   |   -65  |
  // |              |            | `10000000    |  -128  |
  // |--------------|------------|--------------|--------|
  //
  // This makes it pretty easy to get the continuation mask! It's just a single comparison:
  //
  // ```
  // is_continuation = input < -64`
  // ```
  //
  // We can do something similar for the others, but it takes two comparisons instead of one: "is
  // the start of a 4-byte character" is `< -32` and `> -65`, for example. And 2+ bytes is `< 0` and
  // `> -64`. Surely we can do better, they're right next to each other!
  //
  // Getting the is_xxx Masks: Shifting the Range
  // --------------------------------------------
  //
  // Notice *why* continuations were a single comparison. The actual *range* would require two
  // comparisons--`< -64` and `> -129`--but all characters are always greater than -128, so we get
  // that for free. In fact, if we had *unsigned* comparisons, 2+, 3+ and 4+ comparisons would be
  // just as easy: 4+ would be `> 239`, 3+ would be `> 223`, and 2+ would be `> 191`.
  //
  // Instead, we add 128 to each byte, shifting the range up to make comparison easy. This wraps
  // ASCII down into the negative, and puts 4+-Byte Lead at the top:
  //
  // | Type                 | High Bits  | Binary Range | Signed |
  // |----------------------|------------|--------------|-------|
  // | 4+-Byte Lead (+ 127) | `0111`     | `01111111`   |   127 |
  // |                      |            | `01110000    |   112 |
  // |----------------------|------------|--------------|-------|
  // | 3-Byte Lead (+ 127)  | `0110`     | `01101111`   |   111 |
  // |                      |            | `01100000    |    96 |
  // |----------------------|------------|--------------|-------|
  // | 2-Byte Lead (+ 127)  | `010`      | `01011111`   |    95 |
  // |                      |            | `01000000    |    64 |
  // |----------------------|------------|--------------|-------|
  // | Continuation (+ 127) | `00`       | `00111111`   |    63 |
  // |                      |            | `00000000    |     0 |
  // |----------------------|------------|--------------|-------|
  // | ASCII (+ 127)        | `1`        | `11111111`   |    -1 |
  // |                      |            | `10000000`   |  -128 |
  // |----------------------|------------|--------------|-------|
  // 
  // *Now* we can use signed `>` on all of them:
  //
  // ```
  // prev1 = input.prev<1>
  // prev2 = input.prev<2>
  // prev3 = input.prev<3>
  // prev1_flipped = input.prev<1>(prev_input) ^ 0x80; // Same as `+ 128`
  // prev2_flipped = input.prev<2>(prev_input) ^ 0x80; // Same as `+ 128`
  // prev3_flipped = input.prev<3>(prev_input) ^ 0x80; // Same as `+ 128`
  // is_second_byte = prev1_flipped > 63;  // 2+-byte lead
  // is_third_byte  = prev2_flipped > 95;  // 3+-byte lead
  // is_fourth_byte = prev3_flipped > 111; // 4+-byte lead
  // ```
  //
  // NOTE: we use `^ 0x80` instead of `+ 128` in the code, which accomplishes the same thing, and even takes the same number
  // of cycles as `+`, but on many Intel architectures can be parallelized better (you can do 3
  // `^`'s at a time on Haswell, but only 2 `+`'s).
  //
  // That doesn't look like it saved us any instructions, did it? Well, because we're adding the
  // same number to all of them, we can save one of those `+ 128` operations by assembling
  // `prev2_flipped` out of prev 1 and prev 3 instead of assembling it from input and adding 128
  // to it. One more instruction saved!
  //
  // ```
  // prev1 = input.prev<1>
  // prev3 = input.prev<3>
  // prev1_flipped = prev1 ^ 0x80; // Same as `+ 128`
  // prev3_flipped = prev3 ^ 0x80; // Same as `+ 128`
  // prev2_flipped = prev1_flipped.concat<2>(prev3_flipped): // <shuffle: take the first 2 bytes from prev1 and the rest from prev3  
  // ```
  //
  // ### Bringing It All Together: Detecting the Errors
  //
  // At this point, we have `is_continuation`, `is_first_byte`, `is_second_byte` and `is_third_byte`.
  // All we have left to do is check if they match!
  //
  // ```
  // return (is_second_byte | is_third_byte | is_fourth_byte) ^ is_continuation;
  // ```
  //
  // But wait--there's more. The above statement is only 3 operations, but they *cannot be done in
  // parallel*. You have to do 2 `|`'s and then 1 `&`. Haswell, at least, has 3 ports that can do
  // bitwise operations, and we're only using 1!
  //
  // Epilogue: Addition For Booleans
  // -------------------------------
  //
  // There is one big case the above code doesn't explicitly talk about--what if is_second_byte
  // and is_third_byte are BOTH true? That means there is a 3-byte and 2-byte character right next
  // to each other (or any combination), and the continuation could be part of either of them!
  // Our algorithm using `&` and `|` won't detect that the continuation byte is problematic.
  //
  // Never fear, though. If that situation occurs, we'll already have detected that the second
  // leading byte was an error, because it was supposed to be a part of the preceding multibyte
  // character, but it *wasn't a continuation*.
  //
  // We could stop here, but it turns out that we can fix it using `+` and `-` instead of `|` and
  // `&`, which is both interesting and possibly useful (even though we're not using it here). It
  // exploits the fact that in SIMD, a *true* value is -1, and a *false* value is 0. So those
  // comparisons were giving us numbers!
  //
  // Given that, if you do `is_second_byte + is_third_byte + is_fourth_byte`, under normal
  // circumstances you will either get 0 (0 + 0 + 0) or -1 (-1 + 0 + 0, etc.). Thus,
  // `(is_second_byte + is_third_byte + is_fourth_byte) - is_continuation` will yield 0 only if
  // *both* or *neither* are 0 (0-0 or -1 - -1). You'll get 1 or -1 if they are different. Because
  // *any* nonzero value is treated as an error (not just -1), we're just fine here :)
  //
  // Further, if *more than one* multibyte character overlaps,
  // `is_second_byte + is_third_byte + is_fourth_byte` will be -2 or -3! Subtracting `is_continuation`
  // from *that* is guaranteed to give you a nonzero value (-1, -2 or -3). So it'll always be
  // considered an error.
  //
  // One reason you might want to do this is parallelism. ^ and | are not associative, so
  // (A | B | C) ^ D will always be three operations in a row: either you do A | B -> | C -> ^ D, or
  // you do B | C -> | A -> ^ D. But addition and subtraction *are* associative: (A + B + C) - D can
  // be written as `(A + B) + (C - D)`. This means you can do A + B and C - D at the same time, and
  // then adds the result together. Same number of operations, but if the processor can run
  // independent things in parallel (which most can), it runs faster.
  //
  // This doesn't help us on Intel, but might help us elsewhere: on Haswell, at least, | and ^ have
  // a super nice advantage in that more of them can be run at the same time (they can run on 3
  // ports, while + and - can run on 2)! This means that we can do A | B while we're still doing C,
  // saving us the cycle we would have earned by using +. Even more, using an instruction with a
  // wider array of ports can help *other* code run ahead, too, since these instructions can "get
  // out of the way," running on a port other instructions can't.
  // 
  // Epilogue II: One More Trick
  // ---------------------------
  //
  // There's one more relevant trick up our sleeve, it turns out: it turns out on Intel we can "pay
  // for" the (prev<1> + 128) instruction, because it can be used to save an instruction in
  // check_special_cases()--but we'll talk about that there :)
  //
  //
  // Accounting
  // ==========
  //
  // | Type | Operation      | Notes | # ops | Haswell                       | # const ops | Haswell const    |
  // |------|----------------|-------|-------|-------------------------------|-------------|------------------|
  // | SIMD | Bitwise        | & | ^ |     1 | 1*p015                        |             |                  |
  // | SIMD | Add            | + -   |     4 |        1*p15                  |             |                  |
  // | SIMD | Comparison     | < >   |     4 |        1*p15                  |             |                  |
  // | SIMD | Shift          | >> << |       | 	            1*p5             |             |                  |
  // | SIMD | Shuffle        |       |     2 |              1*p5             |             |                  |
  // | SIMD | Load           |       |       |                   1*p237 1*p4 |             | 1*p5             |
  // | SIMD | Splat const    |       |       |                               |           5 | 1*p5             |
  // | SIMD | Load const     |       |       |                               |             |      1*p237 1*p4 |
  // |      | Total          |       |    11 | 1*p015 8*p15 2*p5             |           5 | 5*p5             |
  //
  // This is particularly bad because we have a bunch of shifts and shuffles right next to each other.
  //
  // Paths
  // -----
  // prev3           - Shuffle
  // prev3_flipped                - Bitwise
  // prev2            
  // prev2_flipped   - Shuffle
  // is_third_byte                                       - Comparison
  // is_continuation - Comparison
  // is_second_byte  - Comparison
  // is_fourth_byte                         - Comparison
  // error                        - Add     - Add        - Add        - Add
  //
  // Min Critical Path (5):         Shuffle+Comparison(2) -> Bitwise+Add -> Shuffle+Add+Comparison -> Add+Comparison -> Add
  // Min Critical Path Haswell (5): Shuffle+Comparison -> Comparison+Bitwise+Add -> Shuffle+Comparison -> Add+Comparison -> Add(2)
  //
  really_inline void check_multibyte_lengths(const simd8<uint8_t> input, const simd8<int8_t> prev1_flipped, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
    // Start grabbing this early, we won't need it for a few cycles anyway
    simd8<int8_t> prev2_flipped(prev2 ^ HIGH_BIT);

    // Cont is 10000000-101111111 (-65...-128)
    simd8<uint8_t> is_continuation(simd8<int8_t>(input) < int8_t(-64));

    // 2+-byte lead flipped is 01______ (64-127)
    // 3+-byte lead flipped is 011_____ (96-127)
    // 4+-byte lead flipped is 0111____ (112-127)
    simd8<uint8_t> is_second_byte(prev1_flipped > 63);
    simd8<uint8_t> is_third_byte(prev2_flipped > 95);

    simd8<int8_t> prev3_flipped(prev3 ^ HIGH_BIT);
    simd8<uint8_t> is_fourth_byte(prev3_flipped > 111);
    this->error |= (is_second_byte | is_third_byte | is_fourth_byte) ^ is_continuation;
  }

  //
  // Find errors in bytes 1 and 2 together (one single multi-nibble &)
  //
  // Accounting
  // ==========
  //
  // | Type | Operation      | Notes | # ops | Haswell                       | # const ops | Haswell const    |
  // |------|----------------|-------|-------|-------------------------------|-------------|------------------|
  // | SIMD | Bitwise        | & | ^ |     5 | 1*p015                        |             |                  |
  // | SIMD | Add            | + -   |       |        1*p15                  |             |                  |
  // | SIMD | Comparison     | < >   |       |        1*p15                  |             |                  |
  // | SIMD | Shift          | >> << |     2 | 	            1*p5             |             |                  |
  // | SIMD | Shuffle        |       |     3 |              1*p5             |             |                  |
  // | SIMD | Load           |       |       |                   1*p237 1*p4 |             | 1*p5             |
  // | SIMD | Splat const    |       |       |                               |             | 1*p5             |
  // | SIMD | Load const     |       |       |                               |           3 |      1*p237 1*p4 |
  // |      | Total          |       |    10 | 5*p015       5*p5             |           3 |      3*p237 3*p4 |
  //
  // This is particularly bad because we have a bunch of shifts and shuffles right next to each other.
  // Port 5 is super loaded, man!
  //
  // Paths
  // -----
  // byte_2_high     - Shift      - Bitwise - Shuffle
  // byte_1_high     - Shift      - Bitwise - Shuffle
  // byte_1_low      - Shuffle
  // error                                            - Bitwise - Bitwise - Bitwise
  //
  // Min Critical Path (6): Shift(2)+Shuffle -> Bitwise(2) -> Shuffle(2) -> Bitwise -> Bitwise -> Bitwise
  // Min Critical Path Haswell (7): Shift -> Bitwise+Shift -> Bitwise+Shuffle -> Shuffle -> Bitwise+Shuffle -> Bitwise -> Bitwise
  //
  really_inline void check_special_cases(const simd8<uint8_t> input, const simd8<uint8_t> prev1, const simd8<int8_t> prev1_flipped) {
    //
    // These are the errors we're going to match for bytes 1-2, by looking at the first three
    // nibbles of the character: <high bits of byte 1>> & <low bits of byte 1> & <high bits of byte 2>
    //
    static const int OVERLONG_2  = 0x01;   // 1100000_ 10______ (technically we match 10______ but we could match ________, they both yield errors either way)
    static const int OVERLONG_3  = 0x02;      // 11100000 100_____ ________
    static const int OVERLONG_4  = 0x04;      // 11110000 1000____ ________ ________
    static const int SURROGATE   = 0x08;   // 11101101 [101_]____
    static const int TOO_LARGE   = 0x10;  // 11110100 (1001|101_)____
    static const int TOO_LARGE_2 = 0x20;  // 1111(1___|011_|0101) 10______

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
        CARRY | OVERLONG_3 | SURROGATE,  // ________ [1001]____
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

    // DANGER WILL ROBINSON THIS IS NOT GENERIC, it's a super Intel-focused trick. Generally you
    // have to mask the low bits before doing a table lookup, but on Intel the 16-bit table lookup ignores
    // all bits except the bottom 4 and the high bit. If the high bit is 1, it always returns 0.
    // prev1_flipped will only be 1 if it's ASCII, and we're not detecting any ASCII errors here
    // anyway, so it's all good :)
    const simd8<uint8_t> byte_1_low = prev1_flipped.lookup_16<uint8_t>(
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
    this->error |= byte_1_high & byte_1_low & byte_2_high;
  }

  // Check whether the current bytes are valid UTF-8.
  // At the end of the function, previous gets updated
  // This should come down to 22 instructions if table definitions are in registers--30 if not.
  //
  // Accounting
  // ==========
  //
  // | Type | Operation      | Notes | # ops | Haswell                       | # const ops | Haswell const    |
  // |------|----------------|-------|-------|-------------------------------|-------------|------------------|
  // | SIMD | Bitwise        | & | ^ |     8 | 1*p015                        |             |                  |
  // | SIMD | Add            | + -   |     4 |        1*p15                  |             |                  |
  // | SIMD | Comparison     | < >   |     4 |        1*p15                  |             |                  |
  // | SIMD | Shift          | >> << |     2 | 	            1*p5             |             |                  |
  // | SIMD | Shuffle        |       |     6 |              1*p5             |             |                  |
  // | SIMD | Load           |       |       |                   1*p237 1*p4 |             | 1*p5             |
  // | SIMD | Splat const    |       |       |                               |           1 | 1*p5             |
  // | SIMD | Load const     |       |       |                               |             |      1*p237 1*p4 |
  // |      | Total          |       |    24 | 8*p015 8*p15 8*p5             |             | 1*p5             |
  //
  // Paths
  // -----
  // ### check_utf8_bytes
  //
  // prev1           | Shuffle    |            |            |            |            |            |
  // prev1_flipped   |            | Bitwise    |            |            |            |            |
  //
  // ### check_special_cases
  //
  // byte_2_high     | Shift      | Bitwise    | Shuffle    |            |            |            |
  // byte_1_high     |            | Shift      | Bitwise    | Shuffle    |            |            |
  // byte_1_low      |            |            | Shuffle    |            |            |            |
  // error           |            |            |            | Bitwise    | Bitwise    | Bitwise    |
  // 
  // ### check_multibyte_lengths
  //
  // prev2           | Shuffle    |            |            |            |            |            |
  // prev2_flipped   |            | Bitwise    |            |            |            |            |
  // prev3           | Shuffle    |            |            |            |            |            |
  // prev3_flipped   |            | Bitwise    |            |            |            |            |
  // is_third_byte   |            |            | Comparison |            |            |            |
  // is_second_byte  |            |            | Comparison |            |            |            |
  // is_fourth_byte  |            |            | Comparison |            |            |            |
  // is_continuation | Comparison |            |            |            |            |            |
  // error           |            | Add        |            | Add        | Add        | Add        |
  //
  // Min Critical Path (6): Comparison+Shift+Shuffle(2) -> Bitwise(3)+Add+Shift -> Bitwise+Comparison(2)+Shuffle(3) -> Bitwise+Add+Comparison+Shuffle -> Bitwise+Add -> Bitwise+Add
  //
  // From port usage alone (ignoring the path), Haswell's Min Critical Path is at least 8 (8 shifts+shuffles, all on port 5)
  //
  really_inline void check_utf8_bytes(const simd8<uint8_t> input, simd8<uint8_t> prev1, simd8<uint8_t> prev2, simd8<uint8_t> prev3) {
    // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
    // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
    simd8<int8_t> prev1_flipped(prev1 ^ HIGH_BIT);
    this->check_multibyte_lengths(input, prev1_flipped, prev2, prev3);
    this->check_special_cases(input, prev1, prev1_flipped);
  }

  really_inline bool has_utf8(const simd8x64<uint8_t> input) {
    return input.reduce([&](auto a, auto b) { return a | b; }).any_bits_set_anywhere(HIGH_BIT);
  }

  template<typename T>
  really_inline void check(const simd8x64<uint8_t> input, const uint8_t *buf, T prev) {
    if (unlikely(has_utf8(input))) {
      check_utf8_bytes(input.chunks[0], input.chunks[0].prev<1>(prev), input.chunks[0].prev<2>(prev), prev_input::try_prev_mem<3>(prev, input.chunks[0]));
      for (int i=1; i<simd8x64<uint8_t>::NUM_CHUNKS; i++) {
        check_utf8_bytes(input.chunks[i], input.chunks[i].prev<1>(input.chunks[i-1]), input.chunks[i].prev<2>(input.chunks[i-1]), buf+i*sizeof(simd8<uint8_t>)-3);
      }
      this->prev_incomplete = is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1]);
    } else {
      // If the previous block had incomplete UTF-8 characters at the end, we now know there are no
      // more continuation bytes, so it's an error.
      this->error |= this->prev_incomplete;
    }
  }

  // The only problem that can happen at EOF is that a multibyte character is too short.
  really_inline ErrorValues check_eof() {
    // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
    // possibly finish them.
    this->error |= this->prev_incomplete;
    return this->error.any_bits_set_anywhere() ? UTF8_ERROR : SUCCESS;
  }

}; // struct utf8_checker
