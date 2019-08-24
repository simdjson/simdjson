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

// Transform a simd_input -> simd_input, processing each SIMD register with a SIMD -> SIMD function.
// For example, MAP_CHUNKS(in, _mm_and_epi8(_in, _mm_set1_epi8(' ')))
#define MAP_CHUNKS(A, EXPR) A.map([&](auto _##A) { return (EXPR); })
// Transform a simd_input -> simd_input, processing each SIMD register with a SIMD -> SIMD function
// and calling .to_bitmask() on the result.
// Transform an input-sized chunk into a bitmask, processing each SIMD register and returning a SIMD
// For example, MAP_BITMASK(in, _mm_cmpeq_epi8(_in))
#define MAP_BITMASK(A, EXPR) MAP_CHUNKS(A, EXPR).to_bitmask()
// Transform a pair of simd_inputs -> 1 simd_input, processing each SIMD register pair with a
// (SIMD, SIMD) -> SIMD function.
// For example, MAP_CHUNKS2(a, b, _mm_and_epi8(_a, _b))
#define MAP_CHUNKS2(A, B, EXPR) A.map((B), [&](auto _##A, auto _##B) { return (EXPR); })
// Transform a pair of simd_inputs -> bitmask, processing each SIMD register pair with a
// (SIMD, SIMD) -> SIMD function and calling .to_bitmask() on the result.
// For example, MAP_BITMASK2(a, b, _mm_cmpeq_epi8(_mm_and_epi8(_a, _b), _mm_set1_epi8(' ')))
#define MAP_BITMASK2(A, B, EXPR) MAP_CHUNKS2(A, B, EXPR).to_bitmask()
// Reduce a simd_input -> SIMD register with a (SIMD, SIMD) -> SIMD function.
// For example, REDUCE_CHUNKS(a, b, _mm_or_epi8(_a, _b)) yields a SIMD register where each bit
// is set if any of the bits in any of the chunk SIMD registers are set.
#define REDUCE_CHUNKS(A, EXPR) A.reduce([&](auto _a, auto _b) { return (EXPR); })

} // namespace simdjson

#endif
