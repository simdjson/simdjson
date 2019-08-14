#ifndef SIMDJSON_SIMD_INPUT_H
#define SIMDJSON_SIMD_INPUT_H

#include "simdjson/common_defs.h"
#include "simdjson/portability.h"
#include "simdjson/simdjson.h"
#include <cassert>

namespace simdjson {

template <Architecture> struct simd_input;

// a straightforward comparison of a mask against input.
template <Architecture T>
uint64_t cmp_mask_against_input(simd_input<T> in, uint8_t m);

template <Architecture T> simd_input<T> fill_input(const uint8_t *ptr);

// find all values less than or equal than the content of maxval (using unsigned
// arithmetic)
template <Architecture T>
uint64_t unsigned_lteq_against_input(simd_input<T> in, uint8_t m);

} // namespace simdjson

#endif
