#ifndef SIMDJSON_SIMD_INPUT_H
#define SIMDJSON_SIMD_INPUT_H

#include "simdjson/common_defs.h"
#include "simdjson/portability.h"
#include "simdjson/simdjson.h"
#include <cassert>

namespace simdjson {

template <Architecture>
struct simd_input {
    simd_input(const uint8_t *ptr);
    // a straightforward comparison of a mask against input.
    uint64_t eq(uint8_t m);
    // find all values less than or equal than the content of maxval (using unsigned arithmetic)
    uint64_t lteq(uint8_t m);
}; // struct simd_input

} // namespace simdjson

#endif
