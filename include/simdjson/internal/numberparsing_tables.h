#ifndef SIMDJSON_INTERNAL_NUMBERPARSING_TABLES_H
#define SIMDJSON_INTERNAL_NUMBERPARSING_TABLES_H

#include "simdjson.h"

namespace simdjson {
namespace internal {

// Precomputed powers of ten from 10^0 to 10^22. These
// can be represented exactly using the double type.
extern SIMDJSON_DLLIMPORTEXPORT const double power_of_ten[];
// The mantissas of powers of ten from -308 to 308, extended out to sixty four
// bits. The array contains the powers of ten approximated
// as a 64-bit mantissa. It goes from 10^FASTFLOAT_SMALLEST_POWER to 
// 10^FASTFLOAT_LARGEST_POWER (inclusively). 
// The mantissa is truncated, and
// never rounded up. Uses about 5KB.
extern SIMDJSON_DLLIMPORTEXPORT const uint64_t mantissa_64[];
// A complement to mantissa_64
// complete to a 128-bit mantissa.
// Uses about 5KB but is rarely accessed.
extern SIMDJSON_DLLIMPORTEXPORT const uint64_t mantissa_128[];

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_NUMBERPARSING_TABLES_H
