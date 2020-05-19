#ifndef SIMDJSON_ARM64_NUMBERPARSING_H
#define SIMDJSON_ARM64_NUMBERPARSING_H

#include "simdjson.h"
#include "jsoncharutils.h"
#include "arm64/intrinsics.h"
#include "arm64/bitmanipulation.h"
#include <cmath>
#include <limits>


#ifdef JSON_TEST_NUMBERS // for unit testing
void found_invalid_number(const uint8_t *buf);
void found_integer(int64_t result, const uint8_t *buf);
void found_unsigned_integer(uint64_t result, const uint8_t *buf);
void found_float(double result, const uint8_t *buf);
#endif

namespace simdjson {
namespace arm64 {

// we don't have SSE, so let us use a scalar function
// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
static inline uint32_t parse_eight_digits_unrolled(const char *chars) {
  uint64_t val;
  memcpy(&val, chars, sizeof(uint64_t));
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return uint32_t((val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32);
}

#define SWAR_NUMBER_PARSING

#include "generic/stage2/numberparsing.h"

} // namespace arm64
} // namespace simdjson

#endif // SIMDJSON_ARM64_NUMBERPARSING_H
