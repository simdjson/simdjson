#ifndef SIMDJSON_PPC64_BITMASK_H
#define SIMDJSON_PPC64_BITMASK_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/ppc64/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace ppc64 {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is
// encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_inline uint64_t prefix_xor(uint64_t bitmask) {
  // You can use the version below, however gcc sometimes miscompiles
  // vec_pmsum_be, it happens somewhere around between 8 and 9th version.
  // The performance boost was not noticeable, falling back to a usual
  // implementation.
  //   __vector unsigned long long all_ones = {~0ull, ~0ull};
  //   __vector unsigned long long mask = {bitmask, 0};
  //   // Clang and GCC return different values for pmsum for ull so cast it to one.
  //   // Generally it is not specified by ALTIVEC ISA what is returned by
  //   // vec_pmsum_be.
  // #if defined(__LITTLE_ENDIAN__)
  //   return (uint64_t)(((__vector unsigned long long)vec_pmsum_be(all_ones, mask))[0]);
  // #else
  //   return (uint64_t)(((__vector unsigned long long)vec_pmsum_be(all_ones, mask))[1]);
  // #endif
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
  return bitmask;
}

} // unnamed namespace
} // namespace ppc64
} // namespace simdjson

#endif
