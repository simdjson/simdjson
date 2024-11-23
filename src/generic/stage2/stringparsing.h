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
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
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

simdjson_warn_unused simdjson_inline uint8_t *parse_wobbly_string(const uint8_t *src, uint8_t *dst) {
  // It is not ideal that this function is nearly identical to parse_string.
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
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

simdjson_warn_unused size_t write_string_escaped(const std::string_view input, char *out) noexcept {
  // We are making the following assumption: most strings will either be very short or they will not
  // need escaping.
  size_t i = 0;
  size_t pos = 0;
  if(input.size() >= sizeof(simd8<uint8_t>)) {
    auto vec_processing = [input,out]() -> size_t {
      size_t i = 0;
      size_t pos = 0;
      for(;input.size() - i >= sizeof(simd8<uint8_t>); i += sizeof(simd8<uint8_t>)) {
        simd8<uint8_t> vinput(reinterpret_cast<const uint8_t *>(input.data()) + i);
        // instead of doing it register by register, we could regroup, but consider
        // that we expect most strings to be short.
        if(((vinput <= 31) | (vinput == '\\') | (vinput == '"')).any()) {
          return i; // We have a character that needs escaping
          // We could be more carefully and identify the character that needs escaping.
        }
        vinput.store(reinterpret_cast<uint8_t *>(out) + pos);
        pos += sizeof(simd8<uint8_t>);
      }
      if(i == input.size()) { return input.size(); }
      simd8<uint8_t> vinput(reinterpret_cast<const uint8_t *>(input.data()) + input.size() - sizeof(simd8<uint8_t>));
      if(((vinput <= 31) | (vinput == '\\') | (vinput == '"')).any()) {
        return i; // We have a character that needs escaping
        // We could be more carefully and identify the character that needs escaping.
      }
      vinput.store(reinterpret_cast<uint8_t *>(out) + input.size() - sizeof(simd8<uint8_t>));
      return input.size();
    };
    i = vec_processing();
    pos = i;
    if(i == input.size()) { return pos; }
    // Here we only continue if there was a character that needed escaping.
  }
  static std::string_view control_chars[] = {
    "\\x0000", "\\x0001", "\\x0002", "\\x0003", "\\x0004", "\\x0005", "\\x0006",
    "\\x0007", "\\x0008", "\\t",     "\\n",     "\\x000b", "\\f",     "\\r",
    "\\x000e", "\\x000f", "\\x0010", "\\x0011", "\\x0012", "\\x0013", "\\x0014",
    "\\x0015", "\\x0016", "\\x0017", "\\x0018", "\\x0019", "\\x001a", "\\x001b",
    "\\x001c", "\\x001d", "\\x001e", "\\x001f"};
  static std::array<uint8_t, 256> json_quotable_character =
    []() constexpr {
      std::array<uint8_t, 256> result{};
      for (int i = 0; i < 32; i++) {
        result[i] = 1;
      }
      for (int i : {'"', '\\'}) {
        result[i] = 1;
      }
      return result;
    }();
  // The rest could possibly be vectorized, but consider that we expect most strings
  // to be short or not to require escaping.
  for (; i < input.size(); i++) {
    uint8_t c = static_cast<uint8_t>(input[i]);
    if(json_quotable_character[c]) {
      switch (c) {
      case '"':
        out[pos++] = '\\';
        out[pos++] = '"';
        break;
      case '\\':
        out[pos++] = '\\';
        out[pos++] = '\\';
        break;
      case '\b':
        out[pos++] = '\\';
        out[pos++] = 'b';
        break;
      case '\f':
        out[pos++] = '\\';
        out[pos++] = 'f';
        break;
      case '\n':
        out[pos++] = '\\';
        out[pos++] = 'n';
        break;
      case '\r':
        out[pos++] = '\\';
        out[pos++] = 'r';
        break;
      case '\t':
        out[pos++] = '\\';
        out[pos++] = 't';
        break;
      default:
        control_chars[c].copy(out + pos, 6);
        pos += 6;
      }
    } else {
      out[pos++] = c;
    }
  }
  return pos;
}



} // namespace stringparsing
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE2_STRINGPARSING_H