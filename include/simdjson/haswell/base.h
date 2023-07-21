#ifndef SIMDJSON_HASWELL_BASE_H
#define SIMDJSON_HASWELL_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_HASWELL
namespace simdjson {
/**
 * Implementation for Haswell (Intel AVX2).
 */
namespace haswell {

class implementation;

namespace {
namespace simd {
template <typename T> struct simd8;
template <typename T> struct simd8x64;
} // namespace simd
} // unnamed namespace

} // namespace haswell
} // namespace simdjson

#endif // SIMDJSON_HASWELL_BASE_H
