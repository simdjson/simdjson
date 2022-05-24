namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace utf8_validation {

simdjson_really_inline __m512i check_special_cases(__m512i input, const __m512i prev1) {
  __m512i mask1 = _mm512_setr_epi64(
        0x0202020202020202,
        0x4915012180808080,
        0x0202020202020202,
        0x4915012180808080,
        0x0202020202020202,
        0x4915012180808080,
        0x0202020202020202,
        0x4915012180808080);

    const __m512i v_0f = _mm512_set1_epi8(0x0f);
    __m512i index1 = _mm512_and_si512(_mm512_srli_epi16(prev1, 4), v_0f);

    __m512i byte_1_high = _mm512_shuffle_epi8(mask1, index1);
    __m512i mask2 = _mm512_setr_epi64(
        0xcbcbcb8b8383a3e7,
        0xcbcbdbcbcbcbcbcb,
        0xcbcbcb8b8383a3e7,
        0xcbcbdbcbcbcbcbcb,
        0xcbcbcb8b8383a3e7,
        0xcbcbdbcbcbcbcbcb,
        0xcbcbcb8b8383a3e7,
        0xcbcbdbcbcbcbcbcb);
     __m512i index2 = _mm512_and_si512(prev1, v_0f);

    __m512i byte_1_low = _mm512_shuffle_epi8(mask2, index2);
    __m512i mask3 = _mm512_setr_epi64(
        0x101010101010101,
        0x1010101babaaee6,
        0x101010101010101,
        0x1010101babaaee6,
        0x101010101010101,
        0x1010101babaaee6,
        0x101010101010101,
        0x1010101babaaee6
    );
    __m512i index3 = _mm512_and_si512(_mm512_srli_epi16(input, 4), v_0f);
    __m512i byte_2_high = _mm512_shuffle_epi8(mask3, index3);
    return _mm512_ternarylogic_epi64(byte_1_high, byte_1_low, byte_2_high, 128);
  }

  simdjson_really_inline __m512i check_multibyte_lengths(const __m512i prev2,
      const __m512i prev3, const __m512i sc) {

    __m512i is_third_byte  = _mm512_subs_epu8(prev2, _mm512_set1_epi8(0b11100000u-1)); // Only 111_____ will be > 0
    __m512i is_fourth_byte  = _mm512_subs_epu8(prev3, _mm512_set1_epi8(0b11110000u-1)); // Only 1111____ will be > 0
    __m512i is_third_or_fourth_byte = _mm512_or_si512(is_third_byte, is_fourth_byte);
    const __m512i v_7f = _mm512_set1_epi8(char(0x7f));
    is_third_or_fourth_byte = _mm512_adds_epu8(v_7f, is_third_or_fourth_byte);
    // We want to compute (is_third_or_fourth_byte AND v80) XOR sc.
    const __m512i v_80 = _mm512_set1_epi8(char(0x80));
    return _mm512_ternarylogic_epi32(is_third_or_fourth_byte, v_80, sc, 0b1101010);
    // We could also do it the long way:
    //
    //__m512i is_third_or_fourth_byte_mask = _mm512_and_si512(is_third_or_fourth_byte, v_80);
    //return _mm512_xor_si512(is_third_or_fourth_byte_mask, sc);
  }
  //
  // Return nonzero if there are incomplete multibyte characters at the end of the block:
  // e.g. if there is a 4-byte character, but it's 3 bytes from the end.
  //
  simdjson_really_inline __m512i is_incomplete(const __m512i input) {
    // If the previous input's last 3 bytes match this, they're too short (they ended at EOF):
    // ... 1111____ 111_____ 11______
    const __m512i max_value = _mm512_setr_epi64(
        0xffffffffffffffff,
        0xffffffffffffffff,
        0xffffffffffffffff,
        0xffffffffffffffff,
        0xffffffffffffffff,
        0xffffffffffffffff,
        0xffffffffffffffff,
        0xbfdfefffffffffff);
    return _mm512_subs_epu8(input, max_value);
  }

  struct utf8_checker {
    // If this is nonzero, there has been a UTF-8 error.
    __m512i error{};

    // The last input we received
    __m512i prev_input_block{};
    // Whether the last input we received was incomplete (used for ASCII fast path)
    __m512i prev_incomplete{};

    //
    // Check whether the current bytes are valid UTF-8.
    //
    simdjson_really_inline void check_utf8_bytes(const __m512i input, const __m512i prev_input) {
      // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
      // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
      const __m512i movemask = _mm512_set_epi64(13, 12, 11, 10, 9, 8, 7, 6);
      const __m512i rotated = _mm512_permutex2var_epi64(prev_input, movemask, input);
      __m512i prev1 = _mm512_alignr_epi8(input, rotated, 16-1);
      __m512i prev2 = _mm512_alignr_epi8(input, rotated, 16-2);
      __m512i prev3 = _mm512_alignr_epi8(input, rotated, 16-3);
      __m512i sc = check_special_cases(input, prev1);
      this->error = _mm512_or_si512(check_multibyte_lengths(prev2, prev3, sc), this->error);
    }

    // The only problem that can happen at EOF is that a multibyte character is too short
    // or a byte value too large in the last bytes: check_special_cases only checks for bytes
    // too large in the first of two bytes.
    simdjson_really_inline void check_eof() {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error = _mm512_or_si512(this->error, this->prev_incomplete);
    }

    // returns true if ASCII.
    simdjson_really_inline bool check_next_input(const __m512i input) {
      const __m512i v_80 = _mm512_set1_epi8(char(0x80));
      const __mmask64 ascii = _mm512_test_epi8_mask(input, v_80);
      if(ascii == 0) {
        this->error = _mm512_or_si512(this->error, this->prev_incomplete);
        return true;
      } else {
        this->check_utf8_bytes(input, this->prev_input_block);
        this->prev_incomplete = is_incomplete(input);
        this->prev_input_block = input;
        return false;
      }
    }
    // do not forget to call check_eof!
    simdjson_really_inline error_code errors() {
      return (_mm512_test_epi8_mask(this->error, this->error) != 0) ? error_code::UTF8_ERROR : error_code::SUCCESS;
    }
  }; // struct utf8_checker
} // namespace utf8_validation

using utf8_validation::utf8_checker;

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson