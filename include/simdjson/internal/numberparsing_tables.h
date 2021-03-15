#ifndef SIMDJSON_INTERNAL_NUMBERPARSING_TABLES_H
#define SIMDJSON_INTERNAL_NUMBERPARSING_TABLES_H

#include "simdjson/base.h"

namespace simdjson {
namespace internal {
/**
 * The smallest non-zero float (binary64) is 2^-1074.
 * We take as input numbers of the form w x 10^q where w < 2^64.
 * We have that w * 10^-343  <  2^(64-344) 5^-343 < 2^-1076.
 * However, we have that
 * (2^64-1) * 10^-342 =  (2^64-1) * 2^-342 * 5^-342 > 2^-1074.
 * Thus it is possible for a number of the form w * 10^-342 where
 * w is a 64-bit value to be a non-zero floating-point number.
 *********
 * Any number of form w * 10^309 where w>= 1 is going to be
 * infinite in binary64 so we never need to worry about powers
 * of 5 greater than 308.
 */
constexpr int smallest_power = -342;
constexpr int largest_power = 308;

/**
 * Represents a 128-bit value.
 * low: least significant 64 bits.
 * high: most significant 64 bits.
 */
struct value128 {
  uint64_t low;
  uint64_t high;
};


// Precomputed powers of ten from 10^0 to 10^22. These
// can be represented exactly using the double type.
extern SIMDJSON_DLLIMPORTEXPORT const double power_of_ten[];


/**
 * When mapping numbers from decimal to binary,
 * we go from w * 10^q to m * 2^p but we have
 * 10^q = 5^q * 2^q, so effectively
 * we are trying to match
 * w * 2^q * 5^q to m * 2^p. Thus the powers of two
 * are not a concern since they can be represented
 * exactly using the binary notation, only the powers of five
 * affect the binary significand.
 */


// The truncated powers of five from 5^-342 all the way to 5^308
// The mantissa is truncated to 128 bits, and
// never rounded up. Uses about 10KB.
extern SIMDJSON_DLLIMPORTEXPORT const uint64_t power_of_five_128[];
} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_NUMBERPARSING_TABLES_H
