#ifndef SIMDJSON_STAGE1_FIND_MARKS_FLATTEN_HASWELL_H
#define SIMDJSON_STAGE1_FIND_MARKS_FLATTEN_HASWELL_H

// This file provides the same function as
// stage1_find_marks_flatten.h, but uses Intel intrinsics.
// This should provide better performance on Visual Studio
// and other compilers that do a conservative optimization.

// Specifically, on x64 processors with BMI,
// x & (x - 1) should be mapped to
// the blsr instruction. By using the
// _blsr_u64 intrinsic, we
// ensure that this will happen.
/////////

#include "simdjson/common_defs.h"
#include "simdjson/portability.h"

#ifdef IS_X86_64

TARGET_HASWELL
namespace simdjson {
namespace haswell {

// flatten out values in 'bits' assuming that they are are to have values of idx
// plus their position in the bitvector, and store these indexes at
// base_ptr[base] incrementing base as we go
// will potentially store extra values beyond end of valid bits, so base_ptr
// needs to be large enough to handle this
really_inline void flatten_bits(uint32_t *base_ptr, uint32_t &base,
                                uint32_t idx, uint64_t bits) {
  // In some instances, the next branch is expensive because it is mispredicted.
  // Unfortunately, in other cases,
  // it helps tremendously.
  if (bits == 0)
    return;
  uint32_t cnt = _mm_popcnt_u64(bits);
  uint32_t next_base = base + cnt;
  idx -= 64;
  base_ptr += base;
  {
    base_ptr[0] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[1] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[2] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[3] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[4] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[5] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[6] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[7] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr += 8;
  }
  // We hope that the next branch is easily predicted.
  if (cnt > 8) {
    base_ptr[0] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[1] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[2] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[3] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[4] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[5] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[6] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr[7] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
    base_ptr += 8;
  }
  if (cnt > 16) { // unluckly: we rarely get here
    // since it means having one structural or pseudo-structral element
    // every 4 characters (possible with inputs like "","","",...).
    do {
      base_ptr[0] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr++;
    } while (bits != 0);
  }
  base = next_base;
}
} // namespace haswell
} // namespace simdjson
UNTARGET_REGION
#endif // IS_X86_64
#endif // SIMDJSON_STAGE1_FIND_MARKS_FLATTEN_H
