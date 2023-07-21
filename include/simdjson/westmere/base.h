#ifndef SIMDJSON_WESTMERE_BASE_H
#define SIMDJSON_WESTMERE_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_WESTMERE
namespace simdjson {
/**
 * Implementation for Westmere (Intel SSE4.2).
 */
namespace westmere {

class implementation;

namespace {
namespace simd {

template <typename T> struct simd8;
template <typename T> struct simd8x64;

} // namespace simd
} // unnamed namespace

} // namespace westmere
} // namespace simdjson

#endif // SIMDJSON_WESTMERE_BASE_H
