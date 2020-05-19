#ifndef SIMDJSON_FALLBACK_NUMBERPARSING_H
#define SIMDJSON_FALLBACK_NUMBERPARSING_H

#include "simdjson.h"
#include "jsoncharutils.h"
#include "fallback/bitmanipulation.h"
#include <cmath>
#include <limits>

#ifdef JSON_TEST_NUMBERS // for unit testing
void found_invalid_number(const uint8_t *buf);
void found_integer(int64_t result, const uint8_t *buf);
void found_unsigned_integer(uint64_t result, const uint8_t *buf);
void found_float(double result, const uint8_t *buf);
#endif

namespace simdjson {
namespace fallback {
static inline uint32_t parse_eight_digits_unrolled(const char *chars) {
  uint32_t result = 0;
  for (int i=0;i<8;i++) {
    result = result*10 + (chars[i] - '0');
  }
  return result;
}

#define SWAR_NUMBER_PARSING

#include "generic/stage2/numberparsing.h"

} // namespace fallback

} // namespace simdjson

#endif // SIMDJSON_FALLBACK_NUMBERPARSING_H
