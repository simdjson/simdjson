#ifndef SIMDJSON_PPC64_BITMASK_H
#define SIMDJSON_PPC64_BITMASK_H

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is
// encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_really_inline uint64_t prefix_xor(uint64_t bitmask) {
  __vector unsigned long long all_ones = {~0ull, ~0ull};
  __vector unsigned long long mask = {bitmask, 0};
  // Clang and GCC return different values for pmsum for ull so a bit strange
  // dispatch. Generally it is not specified by ALTIVEC ISA what is returned by
  // vec_pmsum_be.
#if defined(__clang__) && defined(__LITTLE_ENDIAN__)
  return (uint64_t)(((__vector unsigned long long)vec_pmsum_be(all_ones, mask))[0]);
#else
  return (uint64_t)(((__vector unsigned long long)vec_pmsum_be(all_ones, mask))[1]);
#endif
}

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif
