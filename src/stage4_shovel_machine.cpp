#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <immintrin.h>
#include <x86intrin.h>
#endif

#include <cassert>
#include <cstring>

#include "common_defs.h"
#include "simdjson_internal.h"

// they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
// these go into the first 3 buckets of the comparison (1/2/4)

// we are also interested in the four whitespace characters
// space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d

const u32 structural_or_whitespace_negated[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// return non-zero if not a structural or whitespace char
// zero otherwise
really_inline u32 is_not_structural_or_whitespace(u8 c) {
  return structural_or_whitespace_negated[c];
}

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

bool is_hex_digit(u8 v) {
  if (v >= '0' && v <= '9')
    return true;
  v &= 0xdf;
  if (v >= 'A' && v <= 'F')
    return true;
  return false;
}

u8 digit_to_val(u8 v) {
  if (v >= '0' && v <= '9')
    return v - '0';
  v &= 0xdf;
  return v - 'A' + 10;
}

bool hex_to_u32(const u8 *src, u32 *res) {
  u8 v1 = src[0];
  u8 v2 = src[1];
  u8 v3 = src[2];
  u8 v4 = src[3];
  if (!is_hex_digit(v1) || !is_hex_digit(v2) || !is_hex_digit(v3) ||
      !is_hex_digit(v4)) {
    return false;
  }
  *res = digit_to_val(v1) << 24 | digit_to_val(v2) << 16 |
         digit_to_val(v3) << 8 | digit_to_val(v4);
  return true;
}

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
                                ParsedJson &pj, u32 tape_loc) {
  u32 offset = pj.tape[tape_loc] & 0xffffff;
  const u8 *src = &buf[offset + 1]; // we know that buf at offset is a "
  u8 *dst = pj.current_string_buf_loc;
#ifdef DEBUG
  cout << "Entering parse string with offset " << offset << "\n";
#endif
  // basic non-sexy parsing code
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
      pj.current_string_buf_loc = dst + quote_dist + 1;
      pj.tape[tape_loc] =
          ((u32)'"') << 24 |
          (pj.current_string_buf_loc -
           pj.string_buf); // assume 2^24 will hold all strings for now
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
  // later extensions -
  // if \\ we could detect whether it's a substantial run of \ or just eat 2
  // chars and write 1 handle anything short of \u or \\\ (as a prefix) with
  // clever PSHUFB stuff and don't leave SIMD
  return true;
}

#ifdef DOUBLECONV
static StringToDoubleConverter
    converter(StringToDoubleConverter::ALLOW_TRAILING_JUNK, 2000000.0,
              Double::NaN(), NULL, NULL);
#endif

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
                                UNUSED ParsedJson &pj, u32 tape_loc,
                                UNUSED bool found_zero, bool found_minus) {
  u32 offset = pj.tape[tape_loc] & 0xffffff;
////////////////
// This is temporary... but it illustrates how one could use Google's double
// conv.
///
#ifdef DOUBLECONV
  int processed_characters_count;
  double result_double_conv = converter.StringToDouble(
      (const char *)(buf + offset), 10, &processed_characters_count);
  printf("number is %f and used %d chars \n", result_double_conv,
         processed_characters_count);
#endif
  ////////////////
  // end of double conv temporary stuff.
  ////////////////
  if (found_minus) {
    offset++;
  }
  const u8 *src = &buf[offset];
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

  m256 enders_mask = _mm256_set1_epi8(0xe0);
  m256 tmp_enders = _mm256_cmpeq_epi8(_mm256_and_si256(tmp, enders_mask),
                                      _mm256_set1_epi8(0));
  u32 enders = ~(u32)_mm256_movemask_epi8(tmp_enders);
  dumpbits32(enders, "ender characters");

  if (enders == 0) {
    // TODO: scream for help if enders == 0 which means we have
    // a heroically long number string or some garbage
  }
  // TODO: make a mask that indicates where our digits are
  u32 number_mask = ~enders & (enders - 1);
  dumpbits32(number_mask, "number mask");

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
  if (__builtin_popcount(digit_edges) == 1) {
    // try a strtoll
    char *end;
    u64 result = strtoll((const char *)src, &end, 10);
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
    *((u64 *)pj.current_number_buf_loc) = result;
    pj.tape[tape_loc] =
        ((u32)'l') << 24 |
        (pj.current_number_buf_loc -
         pj.number_buf); // assume 2^24 will hold all numbers for now
    pj.current_number_buf_loc += 8;
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
    *((double *)pj.current_number_buf_loc) = result;
    pj.tape[tape_loc] =
        ((u32)'d') << 24 |
        (pj.current_number_buf_loc -
         pj.number_buf); // assume 2^24 will hold all numbers for now
    pj.current_number_buf_loc += 8;
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

  // TODO: if first character is a zero (we know in advance except for -0)
  // second char must be . or eE.

  if (error_sump)
    return true;
  return true;
}

bool tape_disturbed(u32 i, ParsedJson &pj) {
  u32 start_loc = i * MAX_TAPE_ENTRIES;
  u32 end_loc = pj.tape_locs[i];
  return start_loc != end_loc;
}

bool shovel_machine(const u8 *buf, size_t len, ParsedJson &pj) {
  // fixup the mess made by the ape_machine
  // as such it does a bunch of miscellaneous things on the tapes
  u32 error_sump = 0;
  u64 tv = *(const u64 *)"true    ";
  u64 nv = *(const u64 *)"null    ";
  u64 fv = *(const u64 *)"false   ";
  u64 mask4 = 0x00000000ffffffff;
  u64 mask5 = 0x000000ffffffffff;

  // if the tape has been touched at all at the depths outside the safe
  // zone we need to quit. Note that our periodic checks to see that we're
  // inside our safe zone in stage 3 don't guarantee that the system did
  // not get into the danger area briefly.
  if (tape_disturbed(START_DEPTH - 1, pj) ||
      tape_disturbed(REDLINE_DEPTH, pj)) {
    return false;
  }

  // walk over each tape
  for (u32 i = START_DEPTH; i < MAX_DEPTH; i++) {
    u32 start_loc = i * MAX_TAPE_ENTRIES;
    u32 end_loc = pj.tape_locs[i];
    if (start_loc == end_loc) {
      break;
    }
    for (u32 j = start_loc; j < end_loc; j++) {
      switch (pj.tape[j] >> 56) {
      case '{':
      case '[': {
        // pivot our tapes
        // point the enclosing structural char (}]) to the head marker ({[) and
        // put the end of the sequence on the tape at the head marker
        // we start with head marker pointing at the enclosing structural char
        // and the enclosing structural char pointing at the end. Just swap
        // them. also check the balanced-{} or [] property here
        u8 head_marker_c = pj.tape[j] >> 56;
        u32 head_marker_loc = pj.tape[j] & 0xffffffffffffffULL;
        u64 tape_enclosing = pj.tape[head_marker_loc];
        u8 enclosing_c = tape_enclosing >> 56;
        pj.tape[head_marker_loc] = pj.tape[j];
        pj.tape[j] = tape_enclosing;
        error_sump |= (enclosing_c - head_marker_c -
                       2); // [] and {} only differ by 2 chars
        break;
      }
      case '"': {
        error_sump |= !parse_string(buf, len, pj, j);
        break;
      }
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        error_sump |= !parse_number(buf, len, pj, j, false, false);
        break;
      case '0':
        error_sump |= !parse_number(buf, len, pj, j, true, false);
        break;
      case '-':
        error_sump |= !parse_number(buf, len, pj, j, false, true);
        break;
      case 't': {
        u32 offset = pj.tape[j] & 0xffffffffffffffULL;
        const u8 *loc = buf + offset;
        u64 locval; // we want to avoid unaligned 64-bit loads (undefined in
                    // C/C++)
        std::memcpy(&locval, loc, sizeof(u64));
        error_sump |= (locval & mask4) ^ tv;
        error_sump |= is_not_structural_or_whitespace(loc[4]);
        break;
      }
      case 'f': {
        u32 offset = pj.tape[j] & 0xffffffffffffffULL;
        const u8 *loc = buf + offset;
        u64 locval; // we want to avoid unaligned 64-bit loads (undefined in
                    // C/C++)
        std::memcpy(&locval, loc, sizeof(u64));
        error_sump |= (locval & mask5) ^ fv;
        error_sump |= is_not_structural_or_whitespace(loc[5]);
        break;
      }
      case 'n': {
        u32 offset = pj.tape[j] & 0xffffffffffffffULL;
        const u8 *loc = buf + offset;
        u64 locval; // we want to avoid unaligned 64-bit loads (undefined in
                    // C/C++)
        std::memcpy(&locval, loc, sizeof(u64));
        error_sump |= (locval & mask4) ^ nv;
        error_sump |= is_not_structural_or_whitespace(loc[4]);
        break;
      }
      default:
        break;
      }
    }
  }
  if (error_sump) {
    //        cerr << "Ugh!\n";
    return false;
  }
  return true;
}
