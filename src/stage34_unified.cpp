#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <immintrin.h>
#include <x86intrin.h>
#endif

#include <cassert>
#include <cstring>

#include "jsonparser/common_defs.h"
#include "jsonparser/simdjson_internal.h"
#include "jsonparser/jsoncharutils.h"

#include <iostream>
//#define DEBUG
#define PATH_SEP '/'

#if defined(DEBUG) && !defined(DEBUG_PRINTF)
#include <string.h>
#include <stdio.h>
#define DEBUG_PRINTF(format, ...) printf("%s:%s:%d:" format, \
                                         strrchr(__FILE__, PATH_SEP) + 1, \
                                         __func__, __LINE__,  ## __VA_ARGS__)
#elif !defined(DEBUG_PRINTF)
#define DEBUG_PRINTF(format, ...) do { } while(0)
#endif

using namespace std;


// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
const u8 escape_map[256] = {
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

const u32 leading_zeros_to_utf_bytes[33] = {
    1, 1, 1, 1, 1, 1, 1, 1,           // 7 bits for first one
    2, 2, 2, 2,                       // 11 bits for next
    3, 3, 3, 3, 3,                    // 16 bits for next
    4, 4, 4, 4, 4,                    // 21 bits for next
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // error

const u32 UTF_PDEP_MASK[5] = {0x00, // error
                              0x7f, 0x1f3f, 0x0f3f3f, 0x073f3f3f};

const u32 UTF_OR_MASK[5] = {0x00, // error
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

      return true;
    } else if (quote_dist > bs_dist) {
      u8 escape_char = src[bs_dist + 1];
#ifdef DEBUG
      cout << "Found escape char: " << escape_char << "\n";
#endif
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
    }
    return true;
  }
  // can't be reached
  return true;
}





#define NEWPARSENUMBER
#ifdef NEWPARSENUMBER
#include "jsonparser/numberparsing.h"
#else

// does not validation whatsoever, assumes that all digit
// this is CS 101
inline u64 naivestrtoll(const char *p, const char *end) {
    if(p == end) return 0; // should be an error?
    // this code could get a whole lot smarter if we have many long ints:
    u64 x = *p - '0';
    p++;
    for(;p < end;p++) {
      x = (x*10) + (*p - '0');
    }
    return x;
}

// put a parsed version of number (either as a double or a signed long) into the
// number buffer, put a 'tag' indicating which type and where it is back onto
// the tape at that location return false if we can't parse the number which
// means either (a) the number isn't valid, or (b) the number is followed by
// something that isn't whitespace, comma or a close }] character which are the
// only things that should follow a number at this stage bools to detect what we
// found in our initial character already here - we are already switching on 0
// vs 1-9 vs - so we may as well keep separate paths where that's useful

// TODO: see if we really need a separate number_buf or whether we should just
//       have a generic scratch - would need to align before using for this
really_inline bool parse_number(const u8 *buf, UNUSED size_t len,
                                ParsedJson &pj,
                                u32 depth, u32 offset,
                                UNUSED bool found_zero, bool found_minus) {
  if (found_minus) {
    offset++;
  }
  const u8 *src = &buf[offset];
  // this can read past the string content, so we need to have overallocated
  m256 v = _mm256_loadu_si256((const m256 *)(src));
  u64 error_sump = 0;
#ifdef DEBUG
  for (u32 j = 0; j < 32; j++) {
    char c = *(src + j);
    if (isprint(c)) {
      cout << c;
    } else {
      cout << '_';
    }
  }
  cout << "|  ... number handling input\n";
#endif

  // categories to extract
  // Digits:
  // 0 (0x30) - bucket 0
  // 1-9 (never any distinction except if we didn't get the free kick at 0 due
  // to the leading minus) (0x31-0x39) - bucket 1
  // . (0x2e) - bucket 2
  // E or e - no distinction (0x45/0x65) - bucket 3
  // + (0x2b) - bucket 4
  // - (0x2d) - bucket 4
  // Terminators
  // Whitespace: 0x20, 0x09, 0x0a, 0x0d - bucket 5+6
  // Comma and the closes: 0x2c is comma, } is 0x5d, ] is 0x7d - bucket 5+7

  // Another shufti - also a bit hand-hacked. Need to make a better construction
  const m256 low_nibble_mask = _mm256_setr_epi8(
      //  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
      33, 2, 2, 2, 2, 10, 2, 2, 2, 66, 64, 16, 32, 0xd0, 4, 0, 33, 2, 2, 2, 2,
      10, 2, 2, 2, 66, 64, 16, 32, 0xd0, 4, 0);
  const m256 high_nibble_mask = _mm256_setr_epi8(
      //  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
      64, 0, 52, 3, 8, -128, 8, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 64, 0, 52, 3, 8,
      -128, 8, 0x80, 0, 0, 0, 0, 0, 0, 0, 0);

  m256 tmp = _mm256_and_si256(
      _mm256_shuffle_epi8(low_nibble_mask, v),
      _mm256_shuffle_epi8(
          high_nibble_mask,
          _mm256_and_si256(_mm256_srli_epi32(v, 4), _mm256_set1_epi8(0x7f))));
#ifdef DEBUG
  // let us print out the magic:
  uint8_t buffer[32];
  _mm256_storeu_si256((__m256i *)buffer,tmp);
  for(int k = 0; k < 32; k++)
  printf("%.2x ",buffer[k]);
  printf("\n");
#endif
  m256 enders_mask = _mm256_set1_epi8(0xe0);
  m256 tmp_enders = _mm256_cmpeq_epi8(_mm256_and_si256(tmp, enders_mask),
                                      _mm256_set1_epi8(0));
  u32 enders = ~(u32)_mm256_movemask_epi8(tmp_enders);
  dumpbits32(enders, "ender characters");
//dumpbits32_always(enders, "ender characters");

  if (enders == 0) {
    error_sump = 1;
    //  if enders == 0  we have
    // a heroically long number string or some garbage
  }
  // TODO: make a mask that indicates where our digits are // DANIEL: Isn't that digit_characters?
  u32 number_mask = ~enders & (enders - 1);
  dumpbits32(number_mask, "number mask");
//dumpbits32_always(number_mask, "number mask");
  m256 n_mask = _mm256_set1_epi8(0x1f);
  m256 tmp_n =
      _mm256_cmpeq_epi8(_mm256_and_si256(tmp, n_mask), _mm256_set1_epi8(0));
  u32 number_characters = ~(u32)_mm256_movemask_epi8(tmp_n);

  // put something into our error sump if we have something
  // before our ending characters that isn't a valid character
  // for the inside of our JSON
  number_characters &= number_mask;
  error_sump |= number_characters ^ number_mask;
  dumpbits32(number_characters, "number characters");

  m256 d_mask = _mm256_set1_epi8(0x03);
  m256 tmp_d =
      _mm256_cmpeq_epi8(_mm256_and_si256(tmp, d_mask), _mm256_set1_epi8(0));
  u32 digit_characters = ~(u32)_mm256_movemask_epi8(tmp_d);
  digit_characters &= number_mask;
  dumpbits32(digit_characters, "digit characters");
  //  dumpbits32_always(digit_characters, "digit characters");


  m256 p_mask = _mm256_set1_epi8(0x04);
  m256 tmp_p =
      _mm256_cmpeq_epi8(_mm256_and_si256(tmp, p_mask), _mm256_set1_epi8(0));
  u32 decimal_characters = ~(u32)_mm256_movemask_epi8(tmp_p);
  decimal_characters &= number_mask;
  dumpbits32(decimal_characters, "decimal characters");

  m256 e_mask = _mm256_set1_epi8(0x08);
  m256 tmp_e =
      _mm256_cmpeq_epi8(_mm256_and_si256(tmp, e_mask), _mm256_set1_epi8(0));
  u32 exponent_characters = ~(u32)_mm256_movemask_epi8(tmp_e);
  exponent_characters &= number_mask;
  dumpbits32(exponent_characters, "exponent characters");


  m256 zero_mask = _mm256_set1_epi8(0x1);
  m256 tmp_zero =
      _mm256_cmpeq_epi8(tmp, zero_mask);
  u32 zero_characters = (u32)_mm256_movemask_epi8(tmp_zero);
  dumpbits32(zero_characters, "zero characters");

  // if the  zero character is in first position, it
  // needs to be followed by decimal or exponent or ender (note: we
  // handle found_minus separately)
  u32 expo_or_decimal_or_ender = exponent_characters | decimal_characters | enders;
  error_sump |= zero_characters & 0x01 & (~(expo_or_decimal_or_ender >> 1));

  m256 s_mask = _mm256_set1_epi8(0x10);
  m256 tmp_s =
      _mm256_cmpeq_epi8(_mm256_and_si256(tmp, s_mask), _mm256_set1_epi8(0));
  u32 sign_characters = ~(u32)_mm256_movemask_epi8(tmp_s);
  sign_characters &= number_mask;
  dumpbits32(sign_characters, "sign characters");

  u32 digit_edges = ~(digit_characters << 1) & digit_characters;
  dumpbits32(digit_edges, "digit_edges");

  // check that we have 1-3 'edges' only
  u32 t = digit_edges;
  t &= t - 1;
  t &= t - 1;
  t &= t - 1;
  error_sump |= t;

  // check that we start with a digit
  error_sump |= ~digit_characters & 0x1;

  // having done some checks, get lazy and fall back
  // to strtoll or strtod
  // TODO: handle the easy cases ourselves; these are
  // expensive and we've done a lot of the prepwork.
  // return errors if strto* fail, otherwise fill in a code on the tape
  // 'd' for floating point and 'l' for long and put a pointer to the
  // spot in the buffer.
  if ( digit_edges == 1) {
  //if (__builtin_popcount(digit_edges) == 1) { // DANIEL :  shouldn't we have digit_edges == 1
#define NAIVEINTPARSING
#ifdef NAIVEINTPARSING
    // this is faster, maybe, because we use a naive strtoll
    // should be all digits?
    error_sump |= number_characters ^ digit_characters;
    int stringlength = __builtin_ctz(~digit_characters);
    const char *end = (const char *)src + stringlength;
    u64 result = naivestrtoll((const char *)src,end);
    if (found_minus) { // unfortunate that it is a branch?
      result = -result;
    }
#else
    // try a strtoll
    char *end;
    s64 result = strtoll((const char *)src, &end, 10);
    if ((errno != 0) || (end == (const char *)src)) {
      error_sump |= 1;
    }
    error_sump |= is_not_structural_or_whitespace(*end);
    if (found_minus) {
      result = -result;
    }
#endif
#ifdef DEBUG
    cout << "Found number " << result << "\n";
#endif
    pj.write_tape_s64(depth, result);
  } else {
    // try a strtod
    char *end;
    double result = strtod((const char *)src, &end);
    if ((errno != 0) || (end == (const char *)src)) {
      error_sump |= 1;
    }
    error_sump |= is_not_structural_or_whitespace(*end);
    if (found_minus) {
      result = -result;
    }
#ifdef DEBUG
    cout << "Found number " << result << "\n";
#endif
    pj.write_tape_double(depth, result);
    // HACK: return true regardless
    return true; // FIXME: we have a spurious error here
  }
  // TODO: check the MSB element is a digit

  // TODO: a whole bunch of checks

  // TODO:  <=1 decimal point, eE mark, +- construct

  // TODO: first and last character in mask region must be
  // digit

  // TODO: if it exists,
  // Decimal point is after the first cluster of numbers only
  // and before the second cluster of numbers only. It must
  // be digit_or_zero . digit_or_zero strictly

  // TODO: eE mark and +- construct are adjacent with eE first
  // eE mark preceeds final cluster of numbers only
  // and immediately follows second-last cluster of numbers only (not
  // necessarily second, as we may have 4e10).
  // it may suffice to insist that eE is preceeded immediately
  // by a digit of any kind and that it's followed locally by
  // a digit immediately or a +- construct then a digit.

  // TODO: if we have both . and the eE mark then the . must
  // precede the eE mark

  if (error_sump)
    return false;
  return true;
}
#endif

// end copypasta

really_inline bool is_valid_true_atom(const u8 * loc) {
    u64 tv = *(const u64 *)"true    ";
    u64 mask4 = 0x00000000ffffffff;
    u32 error = 0;
    u64 locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
    std::memcpy(&locval, loc, sizeof(u64));
    error = (locval & mask4) ^ tv;
    error |= is_not_structural_or_whitespace(loc[4]);
    return error == 0;
}

really_inline bool is_valid_false_atom(const u8 * loc) {
    u64 fv = *(const u64 *)"false   ";
    u64 mask5 = 0x000000ffffffffff;
    u32 error = 0;
    u64 locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
    std::memcpy(&locval, loc, sizeof(u64));
    error = (locval & mask5) ^ fv;
    error |= is_not_structural_or_whitespace(loc[5]);
    return error == 0;
}

really_inline bool is_valid_null_atom(const u8 * loc) {
    u64 nv = *(const u64 *)"null    ";
    u64 mask4 = 0x00000000ffffffff;
    u32 error = 0;
    u64 locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
    std::memcpy(&locval, loc, sizeof(u64));
    error = (locval & mask4) ^ nv;
    error |= is_not_structural_or_whitespace(loc[4]);
    return error == 0;
}

bool unified_machine(const u8 *buf, size_t len, ParsedJson &pj) {
    u32 i = 0;
    u32 idx;
    u8 c;
    u32 depth = START_DEPTH; // an arbitrary starting depth
    void * ret_address[MAX_DEPTH];

    u32 last_loc = 0; // this is the location of the previous call site; only need one
    // We should also track the tape address of our containing
    // scope for two reasons. First, we will need to put an 
    // up pointer there at each call site so we can navigate
    // upwards. Second, when we encounter the end of the scope
    // we can put the current offset into a record for the 
    // scope so we know where it is

    u32 containing_scope_offset[MAX_DEPTH];

    pj.init();

    // add a sentinel to the end to avoid premature exit
    // need to be able to find the \0 at the 'padded length' end of the buffer
    // FIXME: TERRIFYING!
    size_t j;
    for (j = len; buf[j] != 0; j++)
        ;
    pj.structural_indexes[pj.n_structural_indexes++] = j;

#define UPDATE_CHAR() { idx = pj.structural_indexes[i++]; c = buf[idx]; DEBUG_PRINTF("Got %c at %d (%d offset)\n", c, idx, i-1);}

    // format: call site has 2 entries: 56-bit + '{' or '[' entries pointing first to header then to this location
    //         scope has 2 entries: 56 + '_' entries pointing first to call site then to the last entry in this scope

#define OPEN_SCOPE() { \
    pj.write_saved_loc(last_loc, pj.save_loc(depth), '_'); \
    pj.write_tape(depth, last_loc, '_'); \
    containing_scope_offset[depth] = pj.save_loc(depth); \
    pj.write_tape(depth, 0, '_'); \
    }

#define ESTABLISH_CALLSITE(RETURN_LABEL, SITE_LABEL) { \
    pj.write_tape(depth, containing_scope_offset[depth], c); \
    last_loc = pj.save_loc(depth); \
    pj.write_tape(depth, 0, c); \
    ret_address[depth] = RETURN_LABEL; \
    depth++; \
    goto SITE_LABEL; \
    } 


////////////////////////////// START STATE /////////////////////////////

    DEBUG_PRINTF("at start\n");
    UPDATE_CHAR();
    // do these two speculatively as we will always do
    // them except on fail, in which case it doesn't matter
    ret_address[depth] = &&start_continue;
    containing_scope_offset[depth] = pj.save_loc(depth);
    pj.write_tape(depth, 0, c); // dummy entries

    last_loc = pj.save_loc(depth);
    pj.write_tape(depth, 0, c); // dummy entries
    depth++;

    switch (c) {
        case '{': goto object_begin;
        case '[': goto array_begin;
        default: goto fail;
    }

start_continue:
    // land here after popping our outer object if an object
    DEBUG_PRINTF("in start_object_close\n");
    UPDATE_CHAR();
    switch (c) {
        case 0: goto succeed;
        default: goto fail;
    }

////////////////////////////// OBJECT STATES /////////////////////////////

object_begin:
    DEBUG_PRINTF("in object_begin\n");
    OPEN_SCOPE();
    UPDATE_CHAR();
    switch (c) {
        case '"': {
            if (!parse_string(buf, len, pj, depth, idx)) {
                goto fail;
            }
            goto object_key_state;
        }
        case '}': goto scope_end;
        default: goto fail;
    }

object_key_state:
    DEBUG_PRINTF("in object_key_state\n");
    UPDATE_CHAR();
    if (c != ':') {
        goto fail;
    }
    UPDATE_CHAR();
    switch (c) {
        case '"': {
            if (!parse_string(buf, len, pj, depth, idx)) {
                goto fail;
            }
            break;
        }
        case 't': if (!is_valid_true_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        case 'f': if (!is_valid_false_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        case 'n': if (!is_valid_null_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        case '0': {
            if (!parse_number(buf, len, pj, depth, idx, true, false)) {
                goto fail;
            }
            break;
        }
        case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':  {
            if (!parse_number(buf, len, pj, depth, idx, false, false)) {
                goto fail;
            }
            break;
        }
        case '-': {
            if (!parse_number(buf, len, pj, depth, idx, false, true)) {
                goto fail;
            }
            break;
        }
        case '{': {
            ESTABLISH_CALLSITE(&&object_continue, object_begin);
        }
        case '[': {
            ESTABLISH_CALLSITE(&&object_continue, array_begin);
        }
        default: goto fail;
    }

object_continue:
    DEBUG_PRINTF("in object_continue\n");
    UPDATE_CHAR();
    switch (c) {
        case ',': 
            UPDATE_CHAR();
            if (c != '"') {
                goto fail;
            } else {
                if (!parse_string(buf, len, pj, depth, idx)) {
                    goto fail;
                }
                goto object_key_state;
            }
        case '}': goto scope_end;
        default: goto fail;
    }

////////////////////////////// COMMON STATE /////////////////////////////

scope_end: 
    // write our tape location to the header scope
    pj.write_saved_loc(containing_scope_offset[depth], pj.save_loc(depth), '_');
    depth--;
    // goto saved_state
    goto *ret_address[depth];

    
////////////////////////////// ARRAY STATES /////////////////////////////

array_begin:
    DEBUG_PRINTF("in array_begin\n");
    OPEN_SCOPE();
    // fall through

    UPDATE_CHAR();
    if (c == ']') {
        goto scope_end;
    }

main_array_switch:
    // we call update char on all paths in, so we can peek at c on the
    // on paths that can accept a close square brace (post-, and at start)
    switch (c) {
        case '"': {
            if (!parse_string(buf, len, pj, depth, idx)) {
                goto fail;
            }
            goto array_continue;
        }
        case 't': if (!is_valid_true_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        case 'f': if (!is_valid_false_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        case 'n': if (!is_valid_null_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        
        case '0': {
            if (!parse_number(buf, len, pj, depth, idx, true, false)) {
                goto fail;
            }
            break;
        }
        case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':  {
            if (!parse_number(buf, len, pj, depth, idx, false, false)) {
                goto fail;
            }
            break;
        }
        case '-': {
            if (!parse_number(buf, len, pj, depth, idx, false, true)) {
                goto fail;
            }
            break;
        }
        case '{': {
            ESTABLISH_CALLSITE(&&array_continue, object_begin);
        }
        case '[': {
            ESTABLISH_CALLSITE(&&array_continue, array_begin);
        }
        default: goto fail;
    }

array_continue:
    DEBUG_PRINTF("in array_continue\n");
    UPDATE_CHAR();
    switch (c) {
        case ',': UPDATE_CHAR(); goto main_array_switch;
        case ']': goto scope_end;
        default: goto fail;
    }

////////////////////////////// FINAL STATES /////////////////////////////

succeed:
    DEBUG_PRINTF("in succeed\n");
#ifdef DEBUG
    pj.dump_tapes();
#endif
    return true;
    
fail:
    DEBUG_PRINTF("in fail\n");
#ifdef DEBUG
    pj.dump_tapes();
#endif
    return false;    
}
