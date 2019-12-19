// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "stringparsing.h" (this simplifies amalgation)

// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const uint8_t escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x0c, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
    0, 0, 0x0d, 0, 0x09, 0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x7.

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
};

// handle a unicode codepoint
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
WARN_UNUSED
really_inline bool handle_unicode_codepoint(const uint8_t *block,
                                            int& offset,
                                            uint8_t **dst_ptr) {
  // hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the
  // multilingual plane check
  uint32_t code_point = hex_to_u32_nocheck(&block[offset+2]);
  offset += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if ((block[offset] != '\\') || block[offset+1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = hex_to_u32_nocheck(&block[offset+2]);

    // if the first code point is invalid we will get here, as we will go past
    // the check for being outside the Basic Multilingual plane. If we don't
    // find a \u immediately afterwards we fail out anyhow, but if we do,
    // this check catches both the case of the first code point being invalid
    // or the second code point being invalid.
    if ((code_point | code_point_2) >> 16) {
      return false;
    }

    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    offset += 6;
  }
  size_t dst_offset = codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += dst_offset;
  return dst_offset > 0;
}

WARN_UNUSED
really_inline int parse_string(const uint8_t *block, int offset, ParsedJson &pj) {
  pj.write_tape(pj.current_string_buf_loc - pj.string_buf, '"');
  offset++; /* we know that buf at offset is a " */
  uint8_t *dst = pj.current_string_buf_loc + sizeof(uint32_t);
  const uint8_t *const start_of_string = dst;
  while (1) {
    parse_string_helper helper = find_bs_bits_and_quote_bits(&block[offset], dst);
    if (((helper.bs_bits - 1) & helper.quote_bits) != 0) {
      /* we encountered quotes first. Move dst to point to quotes and exit */

      /* find out where the quote is... */
      auto quote_dist = trailing_zeroes(helper.quote_bits);
      /* Update the block past the string so we don't have to advance through the string again */

      /* NULL termination is still handy if you expect all your strings to
       * be NULL terminated? */
      /* It comes at a small cost */
      dst[quote_dist] = 0;

      uint32_t str_length = (dst - start_of_string) + quote_dist;
      memcpy(pj.current_string_buf_loc, &str_length, sizeof(str_length));
      /*****************************
       * Above, check for overflow in case someone has a crazy string
       * (>=4GB?)                 _
       * But only add the overflow check when the document itself exceeds
       * 4GB
       * Currently unneeded because we refuse to parse docs larger or equal
       * to 4GB.
       ****************************/

      // we advance the point, accounting for NULL termination
      pj.current_string_buf_loc = dst + quote_dist + 1;
      return offset + quote_dist + 1;
    }
    if (((helper.quote_bits - 1) & helper.bs_bits) != 0) {
      /* find out where the backspace is */
      auto bs_dist = trailing_zeroes(helper.bs_bits);
      uint8_t escape_char = block[offset + bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        offset += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(block, offset, &dst)) {
          return 0;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return false; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        offset += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      offset += parse_string_helper::BYTES_PROCESSED;
      dst += parse_string_helper::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return 0;
}
