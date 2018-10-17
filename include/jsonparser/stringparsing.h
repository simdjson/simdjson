#pragma once

#include "common_defs.h"
#include "jsonparser/simdjson_internal.h"
#include "jsonparser/jsoncharutils.h"


// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const u8 escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x12, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
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

static const u32 leading_zeros_to_utf_bytes[33] = {
    1, 1, 1, 1, 1, 1, 1, 1,           // 7 bits for first one
    2, 2, 2, 2,                       // 11 bits for next
    3, 3, 3, 3, 3,                    // 16 bits for next
    4, 4, 4, 4, 4,                    // 21 bits for next
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // error

static const u32 UTF_PDEP_MASK[5] = {0x00, // error
                              0x7f, 0x1f3f, 0x0f3f3f, 0x073f3f3f};

static const u32 UTF_OR_MASK[5] = {0x00, // error
                            0x00, 0xc080, 0xe08080, 0xf0808080};


// handle a unicode codepoint
// write appropriate values into dest
// src will always advance 6 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
really_inline bool handle_unicode_codepoint(const u8 **src_ptr, u8 **dst_ptr) {
  u32 code_point = 0; // read the hex, potentially reading another \u beyond if
                      // it's a // wacky one
  if (!hex_to_u32(*src_ptr + 2, &code_point)) {
    return false;
  }
  *src_ptr += 6;
  // check for the weirdo double-UTF-16 nonsense for things outside Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    // TODO: sanity check and clean up; snippeted from RapidJSON and poorly
    // understood at the moment
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    u32 code_point_2 = 0;
    if (!hex_to_u32(*src_ptr + 2, &code_point_2)) {
      return false;
    }
    if (code_point_2 < 0xdc00 || code_point_2 > 0xdfff) {
      return false;
    }
    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    *src_ptr += 6;
  }
  // TODO: check to see whether the below code is nonsense (it's really only a
  // sketch at this point)
  u32 lz = __builtin_clz(code_point);
  u32 utf_bytes = leading_zeros_to_utf_bytes[lz];
  u32 tmp =
      _pdep_u32(code_point, UTF_PDEP_MASK[utf_bytes]) | UTF_OR_MASK[utf_bytes];
  // swap and move to the other side of the register
  tmp = __builtin_bswap32(tmp);
  tmp >>= ((4 - utf_bytes) * 8) & 31; // if utf_bytes, this could become a shift
                                      // by 32, hence the mask with 31
  // use memcpy to avoid undefined behavior:
  std::memcpy(*(u32 **)dst_ptr, &tmp, sizeof(u32)); //**(u32 **)dst_ptr = tmp;
  *dst_ptr += utf_bytes;
  return true;
}

really_inline bool parse_string(const u8 *buf, UNUSED size_t len,
                                ParsedJson &pj, u32 depth, u32 offset) {
  using namespace std;
  const u8 *src = &buf[offset + 1]; // we know that buf at offset is a "
  u8 *dst = pj.current_string_buf_loc;
#ifdef DEBUG
  cout << "Entering parse string with offset " << offset << "\n";
#endif
  while (1) {
#ifdef DEBUG
    for (u32 j = 0; j < 32; j++) {
      char c = *(src + j);
      if (isprint(c)) {
        cout << c;
      } else {
        cout << '_';
      }
    }
    cout << "|  ... string handling input\n";
#endif
    m256 v = _mm256_loadu_si256((const m256 *)(src));
    u32 bs_bits =
        (u32)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('\\')));
    dumpbits32(bs_bits, "backslash bits 2");
    u32 quote_bits =
        (u32)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('"')));
    dumpbits32(quote_bits, "quote_bits");
#define CHECKUNESCAPED
    // All Unicode characters may be placed within the
    // quotation marks, except for the characters that MUST be escaped:
    // quotation mark, reverse solidus, and the control characters (U+0000
    //through U+001F).
    // https://tools.ietf.org/html/rfc8259
#ifdef CHECKUNESCAPED
    m256 unitsep = _mm256_set1_epi8(0x1F);
    m256 unescaped_vec = _mm256_cmpeq_epi8(_mm256_max_epu8(unitsep,v),unitsep);// could do it with saturated subtraction
#endif // CHECKUNESCAPED
    u32 quote_dist = __builtin_ctz(quote_bits);
    u32 bs_dist = __builtin_ctz(bs_bits);
    // store to dest unconditionally - we can overwrite the bits we don't like
    // later
    _mm256_storeu_si256((m256 *)(dst), v);
#ifdef DEBUG
    cout << "quote dist: " << quote_dist << " bs dist: " << bs_dist << "\n";
#endif

    if (quote_dist < bs_dist) {
#ifdef DEBUG
      cout << "Found end, leaving!\n";
#endif
      // we encountered quotes first. Move dst to point to quotes and exit
      dst[quote_dist] = 0; // null terminate and get out

      pj.write_tape(depth, pj.current_string_buf_loc - pj.string_buf, '"');

      pj.current_string_buf_loc = dst + quote_dist + 1;
#ifdef CHECKUNESCAPED
      // check that there is no unescaped char before the quote
      u32 unescaped_bits = (u32)_mm256_movemask_epi8(unescaped_vec);
      return ((quote_bits - 1) & (~ quote_bits) & unescaped_bits) == 0;
#else  //CHECKUNESCAPED
      return true;
#endif //CHECKUNESCAPED
    } else if (quote_dist > bs_dist) {
      u8 escape_char = src[bs_dist + 1];
#ifdef DEBUG
      cout << "Found escape char: " << escape_char << "\n";
#endif
#ifdef CHECKUNESCAPED
      // we are going to need the unescaped_bits to check for unescaped chars
      u32 unescaped_bits = (u32)_mm256_movemask_epi8(unescaped_vec);
      if(((bs_bits - 1) & (~ bs_bits) & unescaped_bits) != 0) {
        return false;
      }
#endif //CHECKUNESCAPED
      // we encountered backslash first. Handle backslash
      if (escape_char == 'u') {
        // move src/dst up to the start; they will be further adjusted
        // within the unicode codepoint handling code.
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return false;
        }
        return true;
      } else {
        // simple 1:1 conversion. Will eat bs_dist+2 characters in input and
        // write bs_dist+1 characters to output
        // note this may reach beyond the part of the buffer we've actually
        // seen. I think this is ok
        u8 escape_result = escape_map[escape_char];
        if (!escape_result)
          return false; // bogus escape value is an error
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      // they are the same. Since they can't co-occur, it means we encountered
      // neither.
      src += 32;
      dst += 32;
#ifdef CHECKUNESCAPED
      // check for unescaped chars
      if(_mm256_testz_si256(unescaped_vec,unescaped_vec) != 1) {
        return false;
      }
#endif // CHECKUNESCAPED
    }

     // return true; // we would return true if we wanted to stop after 32 characters
  }
  // can't be reached
  return true;
}


