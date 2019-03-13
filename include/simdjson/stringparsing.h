#ifndef SIMDJSON_STRINGPARSING_H
#define SIMDJSON_STRINGPARSING_H

#include "simdjson/common_defs.h"
#include "simdjson/jsoncharutils.h"
#include "simdjson/parsedjson.h"


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
really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr, uint8_t **dst_ptr) {
  // hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the 
  // multilingual plane check
  uint32_t code_point = hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = hex_to_u32_nocheck(*src_ptr + 2);
    
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
    *src_ptr += 6;
  }
  size_t offset = codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}

WARN_UNUSED
really_inline  bool parse_string(const uint8_t *buf, UNUSED size_t len,
                                ParsedJson &pj, UNUSED const uint32_t depth, uint32_t offset) {
#ifdef SIMDJSON_SKIPSTRINGPARSING // for performance analysis, it is sometimes useful to skip parsing
  pj.write_tape(0, '"');// don't bother with the string parsing at all
  return true; // always succeeds
#else
  pj.write_tape(pj.current_string_buf_loc - pj.string_buf, '"');
  const uint8_t *src = &buf[offset + 1]; // we know that buf at offset is a "
  uint8_t *dst = pj.current_string_buf_loc + sizeof(uint32_t);
  const uint8_t *const start_of_string = dst;
  while (1) {
    __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
    // store to dest unconditionally - we can overwrite the bits we don't like
    // later
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), v);
    auto bs_bits =
        static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('\\'))));
    auto quote_mask = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('"'));
    auto quote_bits =
        static_cast<uint32_t>(_mm256_movemask_epi8(quote_mask));
    if(((bs_bits - 1) & quote_bits) != 0 ) {
      // we encountered quotes first. Move dst to point to quotes and exit

      // find out where the quote is...
      uint32_t quote_dist = trailingzeroes(quote_bits);

      // NULL termination is still handy if you expect all your strings to be NULL terminated?
      // It comes at a small cost
      dst[quote_dist] = 0; 

      uint32_t str_length = (dst - start_of_string) + quote_dist; 
      memcpy(pj.current_string_buf_loc,&str_length, sizeof(uint32_t));
      ///////////////////////
      // Above, check for overflow in case someone has a crazy string (>=4GB?)
      // But only add the overflow check when the document itself exceeds 4GB
      // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
      ////////////////////////

      
      // we advance the point, accounting for the fact that we have a NULl termination
      pj.current_string_buf_loc = dst + quote_dist + 1;

#ifdef JSON_TEST_STRINGS // for unit testing
      foundString(buf + offset,start_of_string,pj.current_string_buf_loc - 1);
#endif // JSON_TEST_STRINGS
      return true;
    } 
    if(((quote_bits - 1) & bs_bits ) != 0 ) {
      // find out where the backspace is
      uint32_t bs_dist = trailingzeroes(bs_bits);
      uint8_t escape_char = src[bs_dist + 1];
      // we encountered backslash first. Handle backslash
      if (escape_char == 'u') {
        // move src/dst up to the start; they will be further adjusted
        // within the unicode codepoint handling code.
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
#ifdef JSON_TEST_STRINGS // for unit testing
          foundBadString(buf + offset);
#endif // JSON_TEST_STRINGS
          return false;
        }
      } else {
        // simple 1:1 conversion. Will eat bs_dist+2 characters in input and
        // write bs_dist+1 characters to output
        // note this may reach beyond the part of the buffer we've actually
        // seen. I think this is ok
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
#ifdef JSON_TEST_STRINGS // for unit testing
          foundBadString(buf + offset);
#endif // JSON_TEST_STRINGS
          return false; // bogus escape value is an error
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      // they are the same. Since they can't co-occur, it means we encountered
      // neither.
      src += 32;
      dst += 32;
    }
  }
  // can't be reached
  return true;
#endif // SIMDJSON_SKIPSTRINGPARSING
}


#endif
