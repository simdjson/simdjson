// This file contains a non-architecture-specific version of "flatten" used in stage1.
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

#ifdef SIMDJSON_NAIVE_FLATTEN // useful for benchmarking

// This is just a naive implementation. It should be normally
// disable, but can be used for research purposes to compare
// again our optimized version.
really_inline void flatten_bits(uint32_t *base_ptr, uint32_t &base, uint32_t idx, uint64_t bits) {
  uint32_t *out_ptr = base_ptr + base;
  idx -= 64;
  while (bits != 0) {
    out_ptr[0] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    out_ptr++;
  }
  base = (out_ptr - base_ptr);
}

#else // SIMDJSON_NAIVE_FLATTEN

// flatten out values in 'bits' assuming that they are are to have values of idx
// plus their position in the bitvector, and store these indexes at
// base_ptr[base] incrementing base as we go
// will potentially store extra values beyond end of valid bits, so base_ptr
// needs to be large enough to handle this
really_inline void flatten_bits(uint32_t *&base_ptr, uint32_t idx, uint64_t bits) {
  // In some instances, the next branch is expensive because it is mispredicted.
  // Unfortunately, in other cases,
  // it helps tremendously.
  if (bits == 0)
    return;
  uint32_t cnt = hamming(bits);
  idx -= 64;

  // Do the first 8 all together
  for (int i=0; i<8; i++) {
    base_ptr[i] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
  }

  // Do the next 8 all together (we hope in most cases it won't happen at all
  // and the branch is easily predicted).
  if (cnt > 8) {
    for (int i=8; i<16; i++) {
      base_ptr[i] = idx + trailing_zeroes(bits);
      bits = bits & (bits - 1);
    }
  }

  // Most files don't have 16+ structurals per block, so we take several basically guaranteed
  // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
  // or the start of a value ("abc" true 123) every 4 characters.
  if (cnt > 16) {
    uint32_t i = 16;
    do {
      base_ptr[i] = idx + trailing_zeroes(bits);
      bits = bits & (bits - 1);
      i++;
    } while (i < cnt);
  }
  base_ptr += cnt;
}
#endif // SIMDJSON_NAIVE_FLATTEN
