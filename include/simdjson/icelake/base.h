#ifndef SIMDJSON_ICELAKE_BASE_H
#define SIMDJSON_ICELAKE_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_ICELAKE
namespace simdjson {
/**
 * Implementation for Icelake (Intel AVX512).
 */
namespace icelake {

class implementation;

namespace simd {

template <typename T> struct simd8;
template <> struct simd8<bool>;
template <> struct simd8<uint8_t>;
template <typename T> struct simd8x64;

} // unnamed namespace

} // namespace icelake
} // namespace simdjson

#endif // SIMDJSON_ICELAKE_BASE_H
