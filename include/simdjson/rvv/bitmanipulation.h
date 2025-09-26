#ifndef SIMDJSON_RVV_BITMANIPULATION_H
#define SIMDJSON_RVV_BITMANIPULATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/rvv/base.h"
#endif

namespace simdjson {
namespace rvv {
namespace {

/* result might be undefined when input_num is zero */
simdjson_inline int leading_zeroes(uint64_t input_num) {
#if defined(_MSC_VER) && !defined(__clang__)
    unsigned long leading_zero = 0;
    if (_BitScanReverse64(&leading_zero, input_num))
        return (int)(63 - leading_zero);
    else
        return 64;
#else
    return __builtin_clzll(input_num);
#endif
}

simdjson_inline uint64_t clear_lowest_bit(uint64_t input_num) {
    return input_num & (input_num - 1);
}

simdjson_inline int count_ones(uint64_t input_num) {
    return __builtin_popcountll(input_num);
}


inline simdjson::internal::value128 full_multiplication(uint64_t a, uint64_t b) {
#if __SIZEOF_INT128__
    unsigned __int128 p = (unsigned __int128)a * b;
    return { (uint64_t)p, (uint64_t)(p >> 64) };
#else
    uint64_t lo = a * b;
    uint64_t a0 = (uint32_t)a, a1 = a >> 32;
    uint64_t b0 = (uint32_t)b, b1 = b >> 32;
    uint64_t mid1 = a0 * b1;
    uint64_t mid2 = a1 * b0;
    uint64_t carry = ((mid1 & 0xFFFFFFFF) + (mid2 & 0xFFFFFFFF) + (lo >> 32)) >> 32;
    uint64_t hi = a1 * b1 + (mid1 >> 32) + (mid2 >> 32) + carry;
    return { lo, hi };
#endif
}

} // unnamed namespace
} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_BITMANIPULATION_H