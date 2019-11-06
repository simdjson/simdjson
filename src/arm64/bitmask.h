#ifndef SIMDJSON_ARM64_BITMASK_H
#define SIMDJSON_ARM64_BITMASK_H

#include "simdjson/portability.h"

#ifdef IS_ARM64

#include "haswell/bitmask.h"
#include "simdjson/common_defs.h"

namespace simdjson::arm64 {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
really_inline uint64_t prefix_xor(uint64_t bitmask) {

#ifdef __ARM_FEATURE_CRYPTO // some ARM processors lack this extension
  return vmull_p64(-1ULL, bitmask);
#else
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
  return bitmask;
#endif

}

} // namespace simdjson::arm64
UNTARGET_REGION

#endif // IS_ARM64
#endif
