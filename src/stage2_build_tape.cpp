#include <cassert>
#include <cstring>
#include <iostream>

#include "simdjson/portability.h"
#include "simdjson/common_defs.h"
#include "jsoncharutils.h"

using namespace simdjson;

WARN_UNUSED
really_inline bool is_valid_true_atom(const uint8_t *loc) {
  uint64_t tv = *reinterpret_cast<const uint64_t *>("true    ");
  uint64_t mask4 = 0x00000000ffffffff;
  uint32_t error = 0;
  uint64_t
      locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint64_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask4) ^ tv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_false_atom(const uint8_t *loc) {
  // We have to use an integer constant because the space in the cast
  // below would lead to values illegally being qualified
  // uint64_t fv = *reinterpret_cast<const uint64_t *>("false   ");
  // using this constant (that is the same false) but nulls out the
  // unused bits solves that
  uint64_t fv = 0x00000065736c6166; // takes into account endianness
  uint64_t mask5 = 0x000000ffffffffff;
  // we can't use the 32 bit value for checking for errors otherwise
  // the last character of false (it being 5 byte long!) would be
  // ignored
  uint64_t error = 0;
  uint64_t
      locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint64_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask5) ^ fv;
  error |= is_not_structural_or_whitespace(loc[5]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_null_atom(const uint8_t *loc) {
  uint64_t nv = *reinterpret_cast<const uint64_t *>("null    ");
  uint64_t mask4 = 0x00000000ffffffff;
  uint32_t error = 0;
  uint64_t
      locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint64_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask4) ^ nv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}

#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

#include "arm64/stage2_build_tape.h"
#include "haswell/stage2_build_tape.h"
#include "westmere/stage2_build_tape.h"
