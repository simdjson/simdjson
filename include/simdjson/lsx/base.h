#ifndef SIMDJSON_LSX_BASE_H
#define SIMDJSON_LSX_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
/**
 * Implementation for LSX.
 */
namespace lsx {

class implementation;

namespace {
namespace simd {
template <typename T> struct simd8;
template <typename T> struct simd8x64;
} // namespace simd
} // unnamed namespace

} // namespace lsx
} // namespace simdjson

#endif // SIMDJSON_LSX_BASE_H
