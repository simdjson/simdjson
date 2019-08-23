#ifndef SIMDJSON_SIMD_INPUT_H
#define SIMDJSON_SIMD_INPUT_H

#include "simdjson/common_defs.h"
#include "simdjson/portability.h"
#include "simdjson/simdjson.h"
#include <cassert>

namespace simdjson {

template <Architecture T>
struct simd_input {
    simd_input(const uint8_t *ptr);
    // Map through each simd register in this input, producing another simd_input.
    template <typename F>
    really_inline simd_input<T> map(F const& map_chunk);
    // Map through each simd register across two inputs, producing a single simd_input.
    template <typename F>
    really_inline simd_input<T> map(simd_input<T> b, F const& map_chunk);
    // turn this bytemask (usually the result of a simd comparison operation) into a bitmask.
    uint64_t to_bitmask();
    // a straightforward comparison of a mask against input.
    uint64_t eq(uint8_t m);
    // find all values less than or equal than the content of maxval (using unsigned arithmetic)
    uint64_t lteq(uint8_t m);
}; // struct simd_input

#define MAP_CHUNKS(A, EXPR) A.map([&](auto _##A) { return (EXPR); })
#define MAP_BITMASK(A, EXPR) MAP_CHUNKS(A, EXPR).to_bitmask()
#define MAP_CHUNKS2(A, B, EXPR) A.map((B), [&](auto _##A, auto _##B) { return (EXPR); })
#define MAP_BITMASK2(A, B, EXPR) MAP_CHUNKS2(A, B, EXPR).to_bitmask()

} // namespace simdjson

#endif
