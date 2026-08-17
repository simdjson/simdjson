#include <cstdint>
#ifndef SIMDJSON_SRC_GENERIC_STAGE2_STRINGPARSING_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE2_STRINGPARSING_H
#include <generic/stage2/base.h>
#include <simdjson/generic/jsoncharutils.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
/// @private
namespace stringparsing {

/**
 * Shared accessor logic for `backslash_and_quote`, the per-implementation helper
 * that locates unescaped quotes and backslashes in a processed chunk.
 *
 * Each SIMD kernel keeps only the raw bitmasks (`bs_bits`, `quote_bits`) plus its
 * architecture-specific `copy_and_find()`. The byte-identical position logic
 * below lives here, once, instead of being duplicated once per kernel.
 *
 * These are free-function templates so they resolve correctly for every kernel
 * (the mask type is `uint32_t` where 32 bytes are processed per chunk and
 * `uint64_t` where 64 bytes are processed). The scalar `fallback` implementation
 * has a structurally different `backslash_and_quote` (a single byte rather than
 * bitmasks); it provides its own non-template overloads in its kernel namespace,
 * which overload resolution prefers over these templates.
 */
// `has_quote_first()` is true iff the lowest bit set in quote_bits precedes the
// lowest bit set in bs_bits.
template <typename T>
simdjson_inline bool has_quote_first(const T &b) {
  // This is the number of bits of quotes before the first backslash: if it is
  // nonzero, the first quote comes before the first backslash.
  return ((b.bs_bits - 1) & b.quote_bits) != 0;
}

/**
 * True iff there is at least one backslash in the chunk.
 *
 * `has_quote_first()` and `has_backslash()` are exact complements whenever any
 * backslash or quote is present. The string parsing loops call
 * `has_quote_first()` first and return immediately on a leading quote, so
 * `has_backslash()` is only evaluated when no quote precedes any backslash; in
 * that state the position-aware mask `((quote_bits - 1) & bs_bits)` reduces to
 * `bs_bits != 0`.
 */
template <typename T>
simdjson_inline bool has_backslash(const T &b) { return b.bs_bits != 0; }

// Returns the position (in bytes) of the first quote in the chunk.
template <typename T>
simdjson_inline int quote_index(const T &b) {
  return trailing_zeroes(b.quote_bits);
}

// Returns the position (in bytes) of the first backslash in the chunk.
template <typename T>
simdjson_inline int backslash_index(const T &b) {
  return trailing_zeroes(b.bs_bits);
}

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
simdjson_warn_unused
simdjson_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr, bool allow_replacement) {
  // Use the default Unicode Character 'REPLACEMENT CHARACTER' (U+FFFD)
  constexpr uint32_t substitution_code_point = 0xfffd;
  // jsoncharutils::hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion is not valid; we defer the check for this to inside the
  // multilingual plane check.
  uint32_t code_point = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;

  // If we found a high surrogate, we must
  // check for low surrogate for characters
  // outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    const uint8_t *src_data = *src_ptr;
    /* Compiler optimizations convert this to a single 16-bit load and compare on most platforms */
    if (((src_data[0] << 8) | src_data[1]) != ((static_cast<uint8_t> ('\\') << 8) | static_cast<uint8_t> ('u'))) {
      if(!allow_replacement) { return false; }
      code_point = substitution_code_point;
    } else {
      uint32_t code_point_2 = jsoncharutils::hex_to_u32_nocheck(src_data + 2);

      // We have already checked that the high surrogate is valid and
      // (code_point - 0xd800) < 1024.
      //
      // Check that code_point_2 is in the range 0xdc00..0xdfff
      // and that code_point_2 was parsed from valid hex.
      uint32_t low_bit = code_point_2 - 0xdc00;
      if (low_bit >> 10) {
        if(!allow_replacement) { return false; }
        code_point = substitution_code_point;
      } else {
        code_point =  (((code_point - 0xd800) << 10) | low_bit) + 0x10000;
        *src_ptr += 6;
      }

    }
  } else if (code_point >= 0xdc00 && code_point <= 0xdfff) {
      // If we encounter a low surrogate (not preceded by a high surrogate)
      // then we have an error.
      if(!allow_replacement) { return false; }
      code_point = substitution_code_point;
  }
  size_t offset = jsoncharutils::codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}


// handle a unicode codepoint using the wobbly convention
// https://simonsapin.github.io/wtf-8/
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
simdjson_warn_unused
simdjson_inline bool handle_unicode_codepoint_wobbly(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr) {
  // It is not ideal that this function is nearly identical to handle_unicode_codepoint.
  //
  // jsoncharutils::hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion is not valid; we defer the check for this to inside the
  // multilingual plane check.
  uint32_t code_point = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // If we found a high surrogate, we must
  // check for low surrogate for characters
  // outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    const uint8_t *src_data = *src_ptr;
    /* Compiler optimizations convert this to a single 16-bit load and compare on most platforms */
    if (((src_data[0] << 8) | src_data[1]) == ((static_cast<uint8_t> ('\\') << 8) | static_cast<uint8_t> ('u'))) {
      uint32_t code_point_2 = jsoncharutils::hex_to_u32_nocheck(src_data + 2);
      uint32_t low_bit = code_point_2 - 0xdc00;
      if ((low_bit >> 10) ==  0) {
        code_point =
          (((code_point - 0xd800) << 10) | low_bit) + 0x10000;
        *src_ptr += 6;
      }
    }
  }

  size_t offset = jsoncharutils::codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}


/**
 * Unescape a valid UTF-8 string from src to dst, stopping at a final unescaped quote. There
 * must be an unescaped quote terminating the string. It returns the final output
 * position as pointer. In case of error (e.g., the string has bad escaped codes),
 * then null_ptr is returned. It is assumed that the output buffer is large
 * enough. E.g., if src points at 'joe"', then dst needs to have four free bytes +
 * SIMDJSON_PADDING bytes.
 */
simdjson_warn_unused simdjson_inline uint8_t *parse_string(const uint8_t *src, uint8_t *dst, bool allow_replacement) {
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto b = backslash_and_quote{};
    auto bs_quote = b.copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (has_quote_first(bs_quote)) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + quote_index(bs_quote);
    }
    if (has_backslash(bs_quote)) {
      /* find out where the backspace is */
      auto bs_dist = backslash_index(bs_quote);
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst, allow_replacement)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
}

/**
 * Bounds-safe variant of parse_string for input buffers that are NOT padded to
 * len + SIMDJSON_PADDING bytes. `buf_end` is one past the last readable input
 * byte (buf + len). It behaves exactly like parse_string while we are at least
 * SIMDJSON_PADDING bytes away from buf_end (so every speculative SIMD read stays
 * in bounds); once within the final SIMDJSON_PADDING bytes it copies the few
 * remaining bytes into a space-padded scratch buffer and finishes with the
 * regular parse_string. This keeps the delicate escape/Unicode handling in one
 * place (the proven parse_string) rather than duplicating it.
 *
 * Correctness relies on stage 1 having validated the string, i.e. there is an
 * unescaped closing quote within [src, buf_end); that quote is therefore inside
 * the copied scratch, so parse_string finds it without running off the scratch.
 */
simdjson_warn_unused simdjson_inline uint8_t *parse_string_safe(const uint8_t *src, uint8_t *dst, bool allow_replacement, const uint8_t *buf_end) {
  // Far from the end: identical to parse_string's loop. The guard uses
  // SIMDJSON_PADDING (>= BYTES_PROCESSED) so copy_and_find never reads past
  // buf_end; escape/Unicode look-aheads read within the string (before the
  // closing quote, which is < buf_end), so they are in bounds here too.
  // We add margin (+12) for handle_unicode_codepoint's worst-case lookahead:
  // after seeing a high surrogate, it does hex_to_u32_nocheck on the immediate
  // following bytes (+6 from the '\'), then (if it sees \u) another
  // hex_to_u32_nocheck at +8..+11 relative to the backslash that started the
  // escape. With bs_dist up to BYTES_PROCESSED-1 this reaches +11 from the
  // chunk start. The +12 margin ensures that even on kernels where
  // BYTES_PROCESSED == SIMDJSON_PADDING (e.g. icelake) the 4-byte read stays
  // in-bounds. The scratch fallback (3*PAD) is already safe.
  while (src + SIMDJSON_PADDING + 12 <= buf_end) {
    auto b = backslash_and_quote{};
    auto bs_quote = b.copy_and_find(src, dst);
    if (has_quote_first(bs_quote)) {
      return dst + quote_index(bs_quote);
    }
    if (has_backslash(bs_quote)) {
      auto bs_dist = backslash_index(bs_quote);
      uint8_t escape_char = src[bs_dist + 1];
      if (escape_char == 'u') {
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst, allow_replacement)) {
          return nullptr;
        }
      } else {
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr;
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
  // Within the final SIMDJSON_PADDING bytes: copy what remains into a
  // space-padded scratch (spaces are neither quote nor backslash, so they do not
  // disturb matching) and let the regular parser finish from there. The closing
  // quote is within `remaining` (< SIMDJSON_PADDING), so parse_string finds it in
  // the chunk starting at some offset <= remaining and reads at most
  // BYTES_PROCESSED (<= SIMDJSON_PADDING) further -- i.e. under 2*SIMDJSON_PADDING.
  // We size at 3x for a comfortable margin (the unicode look-ahead reads a few
  // extra bytes past an escape).
  uint8_t scratch[SIMDJSON_PADDING * 3];
  const size_t remaining = size_t(buf_end - src); // < SIMDJSON_PADDING
  std::memset(scratch, ' ', sizeof(scratch));
  std::memcpy(scratch, src, remaining);
  return parse_string(scratch, dst, allow_replacement);
}

simdjson_warn_unused simdjson_inline uint8_t *parse_wobbly_string(const uint8_t *src, uint8_t *dst) {
  // It is not ideal that this function is nearly identical to parse_string.
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto b = backslash_and_quote{};
    auto bs_quote = b.copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (has_quote_first(bs_quote)) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + quote_index(bs_quote);
    }
    if (has_backslash(bs_quote)) {
      /* find out where the backspace is */
      auto bs_dist = backslash_index(bs_quote);
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint_wobbly(&src, &dst)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
}

} // namespace stringparsing

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE2_STRINGPARSING_H