#ifndef SIMDJSON_ARM64_BASE_H
#define SIMDJSON_ARM64_BASE_H

#ifndef SIMDJSON_AMALGAMATED
#include "simdjson/base.h"
#endif // SIMDJSON_AMALGAMATED

namespace simdjson {
/**
 * Implementation for NEON (ARMv8).
 */
namespace arm64 {

class implementation;

namespace {
namespace simd {
template <typename T> struct simd8;
template <typename T> struct simd8x64;
} // namespace simd
} // unnamed namespace

} // namespace arm64
} // namespace simdjson

#endif // SIMDJSON_ARM64_BASE_H
