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

using namespace simd;

struct utf8_checker {
  simd8<uint8_t> has_error;
  simd8<int8_t> prev_carried_continuations;
  simd8<int8_t> prev_high_nibbles;

  // all byte values must be no larger than 0xF4
  really_inline void check_smaller_than_0xF4(simd8<uint8_t> current_bytes) {
    // unsigned, saturates to 0 below max
    this->has_error |= current_bytes.saturating_sub(0xF4u);
  }

  really_inline simd8<int8_t> continuation_lengths(simd8<int8_t> high_nibbles) {
    return high_nibbles.lookup_16<int8_t>(
      1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
      0, 0, 0, 0,             // 10xx (continuation)
      2, 2,                   // 110x
      3,                      // 1110
      4);                     // 1111, next should be 0 (not checked here)
  }

  really_inline simd8<int8_t> carry_continuations(simd8<int8_t> initial_lengths) {
    simd8<int8_t> prev1_carried_continuations = initial_lengths.prev(this->prev_carried_continuations);
    simd8<int8_t> right1 = simd8<int8_t>(simd8<uint8_t>(prev1_carried_continuations).saturating_sub(1));
    simd8<int8_t> sum = initial_lengths + right1;

    simd8<int8_t> prev2_carried_continuations = sum.prev<2>(this->prev_carried_continuations);
    simd8<int8_t> right2 = simd8<int8_t>(simd8<uint8_t>(prev2_carried_continuations).saturating_sub(2));
    return sum + right2;
  }

  really_inline void check_continuations(simd8<int8_t> initial_lengths, simd8<int8_t> carries) {
    // overlap || underlap
    // carry > length && length > 0 || !(carry > length) && !(length > 0)
    // (carries > length) == (lengths > 0)
    // (carries > current) == (current > 0)
    this->has_error |= simd8<uint8_t>(
      (carries > initial_lengths) == (initial_lengths > simd8<int8_t>::zero()));
  }

  really_inline void check_carried_continuations() {
    static const int8_t last_1[32] = {
      9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 1
    };
    this->has_error |= simd8<uint8_t>(this->prev_carried_continuations > simd8<int8_t>(last_1 + 32 - sizeof(simd8<int8_t>)));
  }

  // when 0xED is found, next byte must be no larger than 0x9F
  // when 0xF4 is found, next byte must be no larger than 0x8F
  // next byte must be continuation, ie sign bit is set, so signed < is ok
  really_inline void check_first_continuation_max(simd8<uint8_t> current_bytes,
                                                  simd8<uint8_t> off1_current_bytes) {
    simd8<bool> prev_ED = off1_current_bytes == 0xEDu;
    simd8<bool> prev_F4 = off1_current_bytes == 0xF4u;
    // Check if ED is followed by A0 or greater
    simd8<bool> ED_too_large = (simd8<int8_t>(current_bytes) > simd8<int8_t>::splat(0x9Fu)) & prev_ED;
    // Check if F4 is followed by 90 or greater
    simd8<bool> F4_too_large = (simd8<int8_t>(current_bytes) > simd8<int8_t>::splat(0x8Fu)) & prev_F4;
    // These will also error if ED or F4 is followed by ASCII, but that's an error anyway
    this->has_error |= simd8<uint8_t>(ED_too_large | F4_too_large);
  }

  // map off1_hibits => error condition
  // hibits     off1    cur
  // C       => < C2 && true
  // E       => < E1 && < A0
  // F       => < F1 && < 90
  // else      false && false
  really_inline void check_overlong(simd8<uint8_t> current_bytes,
                                    simd8<uint8_t> off1_current_bytes,
                                    simd8<int8_t> high_nibbles) {
    simd8<int8_t> off1_high_nibbles = high_nibbles.prev(this->prev_high_nibbles);

    // Two-byte characters must start with at least C2
    // Three-byte characters must start with at least E1
    // Four-byte characters must start with at least F1
    simd8<int8_t> initial_mins = off1_high_nibbles.lookup_16<int8_t>(
      -128, -128, -128, -128, -128, -128, -128, -128, // 0xxx -> false
      -128, -128, -128, -128,                         // 10xx -> false
      0xC2, -128,                                     // 1100 -> C2
      0xE1,                                           // 1110
      0xF1                                            // 1111
    );
    simd8<bool> initial_under = initial_mins > simd8<int8_t>(off1_current_bytes);

    // Two-byte characters starting with at least C2 are always OK
    // Three-byte characters starting with at least E1 must be followed by at least A0
    // Four-byte characters starting with at least F1 must be followed by at least 90
    simd8<int8_t> second_mins = off1_high_nibbles.lookup_16<int8_t>(
      -128, -128, -128, -128, -128, -128, -128, -128, -128, // 0xxx => false
      -128, -128, -128,                                     // 10xx => false
      127, 127,                                             // 110x => true
      0xA0,                                                 // 1110
      0x90                                                  // 1111
    );
    simd8<bool> second_under = second_mins > simd8<int8_t>(current_bytes);
    this->has_error |= simd8<uint8_t>(initial_under & second_under);
  }

  really_inline simd8<int8_t> count_nibbles(simd8<uint8_t> bytes) {
    return simd8<int8_t>(bytes.shr<4>());
  }

  // check whether the current bytes are valid UTF-8
  // at the end of the function, previous gets updated
  really_inline void check_utf8_bytes(simd8<uint8_t> bytes, simd8<uint8_t> prev_bytes) {
    simd8<int8_t> high_nibbles = this->count_nibbles(bytes);

    this->check_smaller_than_0xF4(bytes);

    simd8<int8_t> initial_lengths = this->continuation_lengths(high_nibbles);

    simd8<int8_t> carried_continuations = this->carry_continuations(initial_lengths);

    this->check_continuations(initial_lengths, carried_continuations);

    simd8<uint8_t> off1_bytes = bytes.prev(prev_bytes);
    this->check_first_continuation_max(bytes, off1_bytes);

    this->check_overlong(bytes, off1_bytes, high_nibbles);
    this->prev_high_nibbles = high_nibbles;
    this->prev_carried_continuations = carried_continuations;
  }

  really_inline void check(const simd8x64<uint8_t> in, const uint8_t *buf, const simd8<uint8_t> *prev_bytes) {
    simd8<uint8_t> bits = in.reduce([&](auto a, auto b) { return a | b; });
    if (likely(bits.bits_not_set_anywhere(0x80u))) {
      // If it's ascii, we just check carried continuations.
      this->check_carried_continuations();
    } else {
      this->check_utf8_bytes(in.chunks[0], prev_bytes ? *prev_bytes : buf-sizeof(simd8<uint8_t>));
      for (int i=1;i<simd8x64<uint8_t>::NUM_CHUNKS;i++) { this->check_utf8_bytes(in.chunks[i], in.chunks[i-1]); }
    }
  }

  really_inline ErrorValues check_eof() {
    // It's EOF, there will be no more continuations.
    this->check_carried_continuations();
    return this->has_error.any_bits_set_anywhere() ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
  }
}; // struct utf8_checker
