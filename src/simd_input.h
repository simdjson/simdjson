#ifndef SIMDJSON_SIMD_INPUT_H
#define SIMDJSON_SIMD_INPUT_H

#include "simdjson/common_defs.h"
#include "simdjson/portability.h"
#include "simdjson/simdjson.h"

namespace simdjson {

template <Architecture T>
struct simd_input {
    simd_input(const uint8_t *ptr);
    // Run an operation on each chunk.
    template <typename F>
    really_inline void each(F const& each_chunk);
    // Map through each simd register in this input, producing another simd_input.
    template <typename F>
    really_inline simd_input<T> map(F const& map_chunk);
    // Map through each simd register across two inputs, producing a single simd_input.
    template <typename F>
    really_inline simd_input<T> map(simd_input<T> b, F const& map_chunk);
    // Run a horizontal operation like "sum" across the whole input
    // template <typename F>
    // really_inline simd<T> reduce(F const& map_chunk);
    // turn this bytemask (usually the result of a simd comparison operation) into a bitmask.
    uint64_t to_bitmask();
    // a straightforward comparison of a mask against input.
    uint64_t eq(uint8_t m);
    // find all values less than or equal than the content of maxval (using unsigned arithmetic)
    uint64_t lteq(uint8_t m);
}; // struct simd_input

} // namespace simdjson

#endif
