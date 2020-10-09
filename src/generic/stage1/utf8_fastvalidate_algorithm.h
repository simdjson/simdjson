namespace simdjson {
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

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

struct processed_utf_bytes {
  simd8<uint8_t> raw_bytes;
  simd8<int8_t> high_nibbles;
  simd8<int8_t> carried_continuations;
};

struct utf8_checker {
  simd8<uint8_t> has_error;
  processed_utf_bytes previous;

  // all byte values must be no larger than 0xF4
  simdjson_really_inline void check_smaller_than_0xF4(const simd8<uint8_t> current_bytes) {
    // unsigned, saturates to 0 below max
    this->has_error |= current_bytes.saturating_sub(0xF4u);
  }

  simdjson_really_inline simd8<int8_t> continuation_lengths(const simd8<int8_t> high_nibbles) {
    return high_nibbles.lookup_16<int8_t>(
      1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
      0, 0, 0, 0,             // 10xx (continuation)
      2, 2,                   // 110x
      3,                      // 1110
      4);                     // 1111, next should be 0 (not checked here)
  }

  simdjson_really_inline simd8<int8_t> carry_continuations(const simd8<int8_t>& initial_lengths) {
    simd8<int8_t> prev_carried_continuations = initial_lengths.prev(this->previous.carried_continuations);
    simd8<int8_t> right1 = simd8<int8_t>(simd8<uint8_t>(prev_carried_continuations).saturating_sub(1));
    simd8<int8_t> sum = initial_lengths + right1;

    simd8<int8_t> prev2_carried_continuations = sum.prev<2>(this->previous.carried_continuations);
    simd8<int8_t> right2 = simd8<int8_t>(simd8<uint8_t>(prev2_carried_continuations).saturating_sub(2));
    return sum + right2;
  }

  simdjson_really_inline void check_continuations(const simd8<int8_t>& initial_lengths, const simd8<int8_t>& carries) {
    // overlap || underlap
    // carry > length && length > 0 || !(carry > length) && !(length > 0)
    // (carries > length) == (lengths > 0)
    // (carries > current) == (current > 0)
    this->has_error |= simd8<uint8_t>(
      (carries > initial_lengths) == (initial_lengths > simd8<int8_t>::zero()));
  }

  simdjson_really_inline void check_carried_continuations() {
    static const int8_t last_1[32] = {
      9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 1
    };
    this->has_error |= simd8<uint8_t>(this->previous.carried_continuations > simd8<int8_t>(last_1 + 32 - sizeof(simd8<int8_t>)));
  }

  // when 0xED is found, next byte must be no larger than 0x9F
  // when 0xF4 is found, next byte must be no larger than 0x8F
  // next byte must be continuation, ie sign bit is set, so signed < is ok
  simdjson_really_inline void check_first_continuation_max(const simd8<uint8_t> current_bytes,
                                                  const simd8<uint8_t> off1_current_bytes) {
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
  simdjson_really_inline void check_overlong(const simd8<uint8_t> current_bytes,
                                    const simd8<uint8_t> off1_current_bytes,
                                    const simd8<int8_t>& high_nibbles) {
    simd8<int8_t> off1_high_nibbles = high_nibbles.prev(this->previous.high_nibbles);

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

  simdjson_really_inline void count_nibbles(simd8<uint8_t> bytes, struct processed_utf_bytes *answer) {
    answer->raw_bytes = bytes;
    answer->high_nibbles = simd8<int8_t>(bytes.shr<4>());
  }

  // check whether the current bytes are valid UTF-8
  // at the end of the function, previous gets updated
  simdjson_really_inline void check_utf8_bytes(const simd8<uint8_t> current_bytes) {
    struct processed_utf_bytes pb {};
    this->count_nibbles(current_bytes, &pb);

    this->check_smaller_than_0xF4(current_bytes);

    simd8<int8_t> initial_lengths = this->continuation_lengths(pb.high_nibbles);

    pb.carried_continuations = this->carry_continuations(initial_lengths);

    this->check_continuations(initial_lengths, pb.carried_continuations);

    simd8<uint8_t> off1_current_bytes = pb.raw_bytes.prev(this->previous.raw_bytes);
    this->check_first_continuation_max(current_bytes, off1_current_bytes);

    this->check_overlong(current_bytes, off1_current_bytes, pb.high_nibbles);
    this->previous = pb;
  }

  simdjson_really_inline void check_next_input(Dconst simd8<uint8_t> in) {
    if (simdjson_likely(!in.any_bits_set_anywhere(0x80u))) {
      this->check_carried_continuations();
    } else {
      this->check_utf8_bytes(in);
    }
  }

  simdjson_really_inline void check_next_input(const simd8x64<uint8_t>& in) {
    simd8<uint8_t> bits = in.reduce_or();
    if (simdjson_likely(!bits.any_bits_set_anywhere(0x80u))) {
      // it is ascii, we just check carried continuations.
      this->check_carried_continuations();
    } else {
      // it is not ascii so we have to do heavy work
      for (int i=0; i<simd8x64<uint8_t>::NUM_CHUNKS; i++) {
        this->check_utf8_bytes(in.chunks[i]);
      }
    }
  }

  simdjson_really_inline error_code errors() {
    return this->has_error.any_bits_set_anywhere() ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
  }
}; // struct utf8_checker

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
