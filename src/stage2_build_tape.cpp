#include <simdjson.h>
#include <cassert>
#include <cstring>
#include "jsoncharutils.h"
#include "document_parser_callbacks.h"

using namespace simdjson;

WARN_UNUSED
really_inline bool is_valid_true_atom(const uint8_t *loc) {
  uint32_t tv = *reinterpret_cast<const uint32_t *>("true");
  uint32_t error = 0;
  uint32_t
      locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 3 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint32_t));
  error = locval ^ tv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_false_atom(const uint8_t *loc) {
  // assume that loc starts with "f"
  uint32_t fv = *reinterpret_cast<const uint32_t *>("alse");
  uint32_t error = 0;
  uint32_t
      locval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  // this can read up to 4 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc + 1, sizeof(uint32_t));
  error = locval ^ fv;
  error |= is_not_structural_or_whitespace(loc[5]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_null_atom(const uint8_t *loc) {
  uint32_t nv = *reinterpret_cast<const uint32_t *>("null");
  uint32_t error = 0;
  uint32_t
      locval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  // this can read up to 2 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint32_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint32_t));
  error = locval ^ nv;
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
