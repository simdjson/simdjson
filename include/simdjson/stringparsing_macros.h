#ifndef SIMDJSON_STRINGPARSING_MACROS_H
#define SIMDJSON_STRINGPARSING_MACROS_H

// We need to compile that code for multiple architectures. However, target
// attributes can be used only once by function definition. Huge macro seemed
// better than huge code duplication.ç
// bool PARSE_STRING(Architecture T, const uint8_t *buf, size_t len, ParsedJson
//                  &pj,const uint32_t depth, uint32_t offset)
#define PARSE_STRING(T, buf, len, pj, depth, offset)                           \
  {                                                                            \
    pj.write_tape(pj.current_string_buf_loc - pj.string_buf, '"');             \
    const uint8_t *src =                                                       \
        &buf[offset + 1]; /* we know that buf at offset is a " */              \
    uint8_t *dst = pj.current_string_buf_loc + sizeof(uint32_t);               \
    const uint8_t *const start_of_string = dst;                                \
    while (1) {                                                                \
      parse_string_helper helper = find_bs_bits_and_quote_bits<T>(src, dst);   \
      if (((helper.bs_bits - 1) & helper.quote_bits) != 0) {                   \
        /* we encountered quotes first. Move dst to point to quotes and exit   \
         */                                                                    \
                                                                               \
        /* find out where the quote is... */                                   \
        uint32_t quote_dist = trailing_zeroes(helper.quote_bits);              \
                                                                               \
        /* NULL termination is still handy if you expect all your strings to   \
         * be NULL terminated? */                                              \
        /* It comes at a small cost */                                         \
        dst[quote_dist] = 0;                                                   \
                                                                               \
        uint32_t str_length = (dst - start_of_string) + quote_dist;            \
        memcpy(pj.current_string_buf_loc, &str_length, sizeof(uint32_t));      \
        /*****************************                                         \
         * Above, check for overflow in case someone has a crazy string        \
         * (>=4GB?)                 _                                          \
         * But only add the overflow check when the document itself exceeds    \
         * 4GB                                                                 \
         * Currently unneeded because we refuse to parse docs larger or equal  \
         * to 4GB.                                                             \
         ****************************/                                         \
                                                                               \
        /* we advance the point, accounting for the fact that we have a NULL   \
         * termination         */                                              \
        pj.current_string_buf_loc = dst + quote_dist + 1;                      \
        return true;                                                           \
      }                                                                        \
      if (((helper.quote_bits - 1) & helper.bs_bits) != 0) {                   \
        /* find out where the backspace is */                                  \
        uint32_t bs_dist = trailing_zeroes(helper.bs_bits);                    \
        uint8_t escape_char = src[bs_dist + 1];                                \
        /* we encountered backslash first. Handle backslash */                 \
        if (escape_char == 'u') {                                              \
          /* move src/dst up to the start; they will be further adjusted       \
             within the unicode codepoint handling code. */                    \
          src += bs_dist;                                                      \
          dst += bs_dist;                                                      \
          if (!handle_unicode_codepoint(&src, &dst)) {                         \
            return false;                                                      \
          }                                                                    \
        } else {                                                               \
          /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and \
           * write bs_dist+1 characters to output                              \
           * note this may reach beyond the part of the buffer we've actually  \
           * seen. I think this is ok */                                       \
          uint8_t escape_result = escape_map[escape_char];                     \
          if (escape_result == 0u) {                                           \
            return false; /* bogus escape value is an error */                 \
          }                                                                    \
          dst[bs_dist] = escape_result;                                        \
          src += bs_dist + 2;                                                  \
          dst += bs_dist + 1;                                                  \
        }                                                                      \
      } else {                                                                 \
        /* they are the same. Since they can't co-occur, it means we           \
         * encountered neither. */                                             \
        if constexpr (T == Architecture::WESTMERE) {                           \
          src += 16;                                                           \
          dst += 16;                                                           \
        } else {                                                               \
          src += 32;                                                           \
          dst += 32;                                                           \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    /* can't be reached */                                                     \
    return true;                                                               \
  }

#endif