// This file contains a non-architecture-specific version of "flatten" used in stage1.
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

#ifdef TARGETED_ARCHITECTURE
#ifdef TARGETED_REGION

TARGETED_REGION
namespace simdjson {

#ifdef SIMDJSON_NAIVE_FLATTEN // useful for benchmarking
//
// This is just a naive implementation. It should be normally
// disable, but can be used for research purposes to compare
// again our optimized version.
template <>
really_inline void flatten_bits<TARGETED_ARCHITECTURE>(uint32_t *base_ptr, uint32_t &base,
                                                       uint32_t idx, uint64_t bits) {
  uint32_t *out_ptr = base_ptr + base;
  idx -= 64;
  while (bits != 0) {
    out_ptr[0] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    out_ptr++;
  }
  base = (out_ptr - base_ptr);
}

#else
// flatten out values in 'bits' assuming that they are are to have values of idx
// plus their position in the bitvector, and store these indexes at
// base_ptr[base] incrementing base as we go
// will potentially store extra values beyond end of valid bits, so base_ptr
// needs to be large enough to handle this
template<>
really_inline void flatten_bits<TARGETED_ARCHITECTURE>(uint32_t *base_ptr, uint32_t &base,
                                                       uint32_t idx, uint64_t bits) {
  // In some instances, the next branch is expensive because it is mispredicted.
  // Unfortunately, in other cases,
  // it helps tremendously.
  if (bits == 0)
    return;
  uint32_t cnt = hamming(bits);
  uint32_t next_base = base + cnt;
  idx -= 64;
  base_ptr += base;
  {
    base_ptr[0] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[1] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[2] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[3] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[4] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[5] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[6] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[7] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr += 8;
  }
  // We hope that the next branch is easily predicted.
  if (cnt > 8) {
    base_ptr[0] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[1] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[2] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[3] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[4] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[5] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[6] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[7] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    base_ptr += 8;
  }
  if (cnt > 16) { // unluckly: we rarely get here
    // since it means having one structural or pseudo-structral element
    // every 4 characters (possible with inputs like "","","",...).
    do {
      base_ptr[0] = idx + trailing_zeroes(bits);
      bits = bits & (bits - 1);
      base_ptr++;
    } while (bits != 0);
  }
  base = next_base;
}
#endif // SIMDJSON_NAIVE_FLATTEN

} // namespace simdjson
UNTARGET_REGION

#else
#error TARGETED_REGION must be specified before including.
#endif // TARGETED_REGION
#else
#error TARGETED_ARCHITECTURE must be specified before including.
#endif // TARGETED_ARCHITECTURE
