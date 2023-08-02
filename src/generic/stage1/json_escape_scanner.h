#ifndef SIMDJSON_SRC_GENERIC_STAGE1_JSON_ESCAPE_SCANNER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_JSON_ESCAPE_SCANNER_H
#include <generic/stage1/base.h>
#include <generic/stage1/buf_block_reader.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

/**
 * Scans for escape characters in JSON, taking care with multiple backslashes (\\n vs. \n).
 */
struct json_escape_scanner {
  /** The actual escape characters (the backslashes themselves). */
  uint64_t next_is_escaped = 0ULL;

  struct escaped_and_escape {
    /**
     * Mask of escaped characters.
     *
     * ```
     * \n \\n \\\n \\\\n \
     * 0100100010100101000
     *  n  \   \ n  \ \
     * ```
     */
    uint64_t escaped;
    /**
     * Mask of escape characters.
     *
     * ```
     * \n \\n \\\n \\\\n \
     * 1001000101001010001
     * \  \   \ \  \ \   \
     * ```
     */
    uint64_t escape;
  };

  /**
   * Get a mask of both escape and escaped characters (the characters following a backslash).
   *
   * @param potential_escape A mask of the character that can escape others (but could be
   *        escaped itself). e.g. block.eq('\\')
   */
  simdjson_really_inline escaped_and_escape next(uint64_t backslash) noexcept {

#if !SIMDJSON_SKIP_BACKSLASH_SHORT_CIRCUIT
    if (!backslash) { return {next_escaped_without_backslashes(), 0}; }
#endif

    // |                                | Mask (shows characters instead of 1's) | Depth | Instructions        |
    // |--------------------------------|----------------------------------------|-------|---------------------|
    // | string                         | `\\n_\\\n___\\\n___\\\\___\\\\__\\\`   |       |                     |
    // |                                | `    even   odd    even   odd   odd`   |       |                     |
    // | potential_escape               | ` \  \\\    \\\    \\\\   \\\\  \\\`   | 1     | 1 (backslash & ~first_is_escaped)
    // | escape_and_terminal_code       | ` \n \ \n   \ \n   \ \    \ \   \ \`   | 5     | 5 (next_escape_and_terminal_code())
    // | escaped                        | `\    \ n    \ n    \ \    \ \   \ ` X | 6     | 7 (escape_and_terminal_code ^ (potential_escape | first_is_escaped))
    // | escape                         | `    \ \    \ \    \ \    \ \   \ \`   | 6     | 8 (escape_and_terminal_code & backslash)
    // | first_is_escaped               | `\                                 `   | 7 (*) | 9 (escape >> 63) ()
    //                                                                               (*) this is not needed until the next iteration
    uint64_t escape_and_terminal_code = next_escape_and_terminal_code(backslash & ~this->next_is_escaped);
    uint64_t escaped = escape_and_terminal_code ^ (backslash | this->next_is_escaped);
    uint64_t escape = escape_and_terminal_code & backslash;
    this->next_is_escaped = escape >> 63;
    return {escaped, escape};
  }

private:
  static constexpr const uint64_t ODD_BITS = 0xAAAAAAAAAAAAAAAAULL;

  simdjson_really_inline uint64_t next_escaped_without_backslashes() noexcept {
    uint64_t escaped = this->next_is_escaped;
    this->next_is_escaped = 0;
    return escaped;
  }

  /**
   * Returns a mask of the next escape characters (masking out escaped backslashes), along with
   * any non-backslash escape codes.
   *
   * \n \\n \\\n \\\\n returns:
   * \n \   \ \n \ \
   * 11 100 1011 10100
   *
   * You are expected to mask out the first bit yourself if the previous block had a trailing
   * escape.
   *
   * & the result with potential_escape to get just the escape characters.
   * ^ the result with (potential_escape | first_is_escaped) to get escaped characters.
   */
  static simdjson_really_inline uint64_t next_escape_and_terminal_code(uint64_t potential_escape) noexcept {
    // If we were to just shift and mask out any odd bits, we'd actually get a *half* right answer:
    // any even-aligned backslash runs would be correct! Odd-aligned backslash runs would be
    // inverted (\\\ would be 010 instead of 101).
    //
    // ```
    // string:              | ____\\\\_\\\\_____ |
    // maybe_escaped | ODD  |     \ \   \ \      |
    //               even-aligned ^^^  ^^^^ odd-aligned
    // ```
    //
    // Taking that into account, our basic strategy is:
    //
    // 1. Use subtraction to produce a mask with 1's for even-aligned runs and 0's for
    //    odd-aligned runs.
    // 2. XOR all odd bits, which masks out the odd bits in even-aligned runs, and brings IN the
    //    odd bits in odd-aligned runs.
    // 3. & with backslash to clean up any stray bits.
    // runs are set to 0, and then XORing with "odd":
    //
    // |                                | Mask (shows characters instead of 1's) | Instructions        |
    // |--------------------------------|----------------------------------------|---------------------|
    // | string                         | `\\n_\\\n___\\\n___\\\\___\\\\__\\\`   |
    // |                                | `    even   odd    even   odd   odd`   |
    // | maybe_escaped                  | `  n  \\n    \\n    \\\_   \\\_  \\` X | 1 (potential_escape << 1)
    // | maybe_escaped_and_odd          | ` \n_ \\n _ \\\n_ _ \\\__ _\\\_ \\\`   | 1 (maybe_escaped | odd)
    // | even_series_codes_and_odd      | `  n_\\\  _    n_ _\\\\ _     _    `   | 1 (maybe_escaped_and_odd - potential_escape)
    // | escape_and_terminal_code       | ` \n \ \n   \ \n   \ \    \ \   \ \`   | 1 (^ odd)
    //

    // Escaped characters are characters following an escape.
    uint64_t maybe_escaped = potential_escape << 1;

    // To distinguish odd from even escape sequences, therefore, we turn on any *starting*
    // escapes that are on an odd byte. (We actually bring in all odd bits, for speed.)
    // - Odd runs of backslashes are 0000, and the code at the end ("n" in \n or \\n) is 1.
    // - Odd runs of backslashes are 1111, and the code at the end ("n" in \n or \\n) is 0.
    // - All other odd bytes are 1, and even bytes are 0.
    uint64_t maybe_escaped_and_odd_bits     = maybe_escaped | ODD_BITS;
    uint64_t even_series_codes_and_odd_bits = maybe_escaped_and_odd_bits - potential_escape;

    // Now we flip all odd bytes back with xor. This:
    // - Makes odd runs of backslashes go from 0000 to 1010
    // - Makes even runs of backslashes go from 1111 to 1010
    // - Sets actually-escaped codes to 1 (the n in \n and \\n: \n = 11, \\n = 100)
    // - Resets all other bytes to 0
    return even_series_codes_and_odd_bits ^ ODD_BITS;
  }
};

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_JSON_STRING_SCANNER_H