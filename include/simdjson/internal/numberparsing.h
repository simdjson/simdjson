#ifndef SIMDJSON_INTERNAL_NUMBERPARSING_H
#define SIMDJSON_INTERNAL_NUMBERPARSING_H

#include "simdjson/base.h"

namespace simdjson {
namespace internal {

/**
 * The value of the eight ASCII digits in val, the first digit in the lowest
 * byte (little-endian). Only the low four bits of each byte count, so zero
 * bytes act as leading zeros. The scalar parse_eight_digits_unrolled.
 * credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
 */
simdjson_inline uint32_t parse_eight_digits_swar(uint64_t val) {
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return static_cast<uint32_t>((val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32);
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_NUMBERPARSING_H
