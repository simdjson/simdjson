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
  simd8<uint8_t> first_len;
};

struct utf8_checker {
  simd8<bool> has_error;
  processed_utf_bytes previous;

  simdjson_really_inline void check_carried_continuations() {
    static const int8_t last_len[32] = {
      9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 2, 1, 0
    };
    this->has_error |= simd8<int8_t>(this->previous.first_len) > simd8<int8_t>(last_len + 32 - sizeof(simd8<int8_t>));
  }

  // check whether the current bytes are valid UTF-8
  // at the end of the function, previous gets updated
  simdjson_really_inline void check_utf8_bytes(const simd8<uint8_t> current_bytes) {

    /* high_nibbles = input >> 4 */
    const simd8<uint8_t> high_nibbles = current_bytes.shr<4>();

    /*
    * Map high nibble of "First Byte" to legal character length minus 1
    * 0x00 ~ 0xBF --> 0
    * 0xC0 ~ 0xDF --> 1
    * 0xE0 ~ 0xEF --> 2
    * 0xF0 ~ 0xFF --> 3
    */
    /* first_len = legal character length minus 1 */
    /* 0 for 00~7F, 1 for C0~DF, 2 for E0~EF, 3 for F0~FF */
    /* first_len = first_len_tbl[high_nibbles] */
    simd8<uint8_t> first_len = high_nibbles.lookup_16<uint8_t>(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3);

    /* Map "First Byte" to 8-th item of range table (0xC2 ~ 0xF4) */
    /* First Byte: set range index to 8 for bytes within 0xC0 ~ 0xFF */
    /* range = first_range_tbl[high_nibbles] */
    simd8<uint8_t> range     = high_nibbles.lookup_16<uint8_t>(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8);

    /* Second Byte: set range index to first_len */
    /* 0 for 00~7F, 1 for C0~DF, 2 for E0~EF, 3 for F0~FF */
    /* range |= (first_len, previous->first_len) << 1 byte */
    range |= first_len.prev(this->previous.first_len);

    /* Third Byte: set range index to saturate_sub(first_len, 1) */
    /* 0 for 00~7F, 0 for C0~DF, 1 for E0~EF, 2 for F0~FF */
    /* range |= (first_len - 1) << 2 bytes */
    range |= first_len.saturating_sub(1).prev<2>(this->previous.first_len.saturating_sub(1));

    /* Fourth Byte: set range index to saturate_sub(first_len, 2) */
    /* 0 for 00~7F, 0 for C0~DF, 0 for E0~EF, 1 for F0~FF */
    /* range |= (first_len - 2) << 3 bytes */
    range |= first_len.saturating_sub(2).prev<3>(this->previous.first_len.saturating_sub(2));

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
    /* Overlaps lead to index 9~15, which are illegal in range table */
    /* shift1 = (input, previous->input) << 1 byte */
    simd8<uint8_t> shift1 = current_bytes.prev(this->previous.raw_bytes);
    /*
      * shift1:  | EF  F0 ... FE | FF  00  ... ...  DE | DF  E0 ... EE |
      * pos:     | 0   1      15 | 16  17           239| 240 241    255|
      * pos-240: | 0   0      0  | 0   0            0  | 0   1      15 |
      * pos+112: | 112 113    127|       >= 128        |     >= 128    |
      */
    simd8<uint8_t> pos = shift1 - 0xEF;

    /*
    * Tables for fast handling of four special First Bytes(E0,ED,F0,F4), after
    * which the Second Byte are not 80~BF. It contains "range index adjustment".
    * +------------+---------------+------------------+----------------+
    * | First Byte | original range| range adjustment | adjusted range |
    * +------------+---------------+------------------+----------------+
    * | E0         | 2             | 2                | 4              |
    * +------------+---------------+------------------+----------------+
    * | ED         | 2             | 3                | 5              |
    * +------------+---------------+------------------+----------------+
    * | F0         | 3             | 3                | 6              |
    * +------------+---------------+------------------+----------------+
    * | F4         | 4             | 4                | 8              |
    * +------------+---------------+------------------+----------------+
    */
    /* index1 -> E0, index14 -> ED */
    simd8<uint8_t> range2 = pos.saturating_sub(240).lookup_16<uint8_t>(0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0);
    /* index1 -> F0, index5 -> F4 */
    range2 += pos.saturating_add(112).lookup_16<uint8_t>(0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    range += range2;

    /* Load min and max values per calculated range index */
    /*
    * Range table, map range index to min and max values
    * Index 0    : 00 ~ 7F (First Byte, ascii)
    * Index 1,2,3: 80 ~ BF (Second, Third, Fourth Byte)
    * Index 4    : A0 ~ BF (Second Byte after E0)
    * Index 5    : 80 ~ 9F (Second Byte after ED)
    * Index 6    : 90 ~ BF (Second Byte after F0)
    * Index 7    : 80 ~ 8F (Second Byte after F4)
    * Index 8    : C2 ~ F4 (First Byte, non ascii)
    * Index 9~15 : illegal: i >= 127 && i <= -128
    */
    simd8<uint8_t> minv = range.lookup_16<uint8_t>(
      0x00, 0x80, 0x80, 0x80, 0xA0, 0x80, 0x90, 0x80,
      0xC2, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F
    );
    simd8<uint8_t> maxv = range.lookup_16<uint8_t>(
      0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x9F, 0xBF, 0x8F,
      0xF4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
    );

    // We're fine with high-bit wraparound here, so we use int comparison since it's faster on Intel
    this->has_error |= simd8<int8_t>(minv) > simd8<int8_t>(current_bytes);
    this->has_error |= simd8<int8_t>(current_bytes) > simd8<int8_t>(maxv);

    this->previous.raw_bytes = current_bytes;
    this->previous.first_len = first_len;
  }

  simdjson_really_inline void check_next_input(const simd8<uint8_t> in) {
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
    return this->has_error.any() ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
  }
}; // struct utf8_checker

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
