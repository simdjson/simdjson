#ifndef SIMDJSON_RVV_BASE_H
#define SIMDJSON_RVV_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE
namespace simdjson {
namespace rvv {

class implementation;

namespace {
namespace simd {
template <typename T> struct simd8;
template <typename T> struct simd8x64;
} // namespace simd
} // unnamed namespace
} // namespace rvv
} // namespace simdjson
#endif // SIMDJSON_RVV_BASE_H