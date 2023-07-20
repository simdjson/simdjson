#ifndef SIMDJSON_HASWELL_BITMASK_H
#define SIMDJSON_HASWELL_BITMASK_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/haswell/base.h"
#include "simdjson/haswell/intrinsics.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace haswell {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_inline uint64_t prefix_xor(const uint64_t bitmask) {
  // There should be no such thing with a processor supporting avx2
  // but not clmul.
  __m128i all_ones = _mm_set1_epi8('\xFF');
  __m128i result = _mm_clmulepi64_si128(_mm_set_epi64x(0ULL, bitmask), all_ones, 0);
  return _mm_cvtsi128_si64(result);
}

} // unnamed namespace
} // namespace haswell
} // namespace simdjson

#endif // SIMDJSON_HASWELL_BITMASK_H
