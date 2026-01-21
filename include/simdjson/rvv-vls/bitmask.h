#ifndef SIMDJSON_RVV_VLS_BITMASK_H
#define SIMDJSON_RVV_VLS_BITMASK_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/rvv-vls/base.h"
#include "simdjson/rvv-vls/intrinsics.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace rvv_vls {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_inline uint64_t prefix_xor(uint64_t bitmask) {
#if __riscv_zbc
  return __riscv_clmul_64(bitmask, ~(uint64_t)0);
#elif __riscv_zvbc
  return __riscv_vmv_x(__riscv_vclmul(__riscv_vmv_s_x_u64m1(bitmask, 1), ~(uint64_t)0, 1));
#else
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
#endif
  return bitmask;
}

} // unnamed namespace
} // namespace rvv_vls
} // namespace simdjson

#endif // SIMDJSON_RVV_VLS_BITMASK_H

