#ifndef SIMDJSON_STAGE2_BUILD_TAPE_H
#define SIMDJSON_STAGE2_BUILD_TAPE_H

#include <cassert>
#include <cstring>
#include <iostream>

#include "simdjson/common_defs.h"
#include "simdjson/jsoncharutils.h"
#include "simdjson/numberparsing.h"
#include "simdjson/parsedjson.h"
#include "simdjson/simdjson.h"
#include "simdjson/stringparsing.h"

namespace simdjson {
void init_state_machine();

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

template <Architecture T = Architecture::NATIVE>
WARN_UNUSED ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER LENIENT_MEM_SANITIZER int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj);

template <Architecture T = Architecture::NATIVE>
int unified_machine(const char *buf, size_t len, ParsedJson &pj) {
  return unified_machine<T>(reinterpret_cast<const uint8_t *>(buf), len, pj);
}

} // namespace simdjson

#endif
