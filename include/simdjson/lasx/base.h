#ifndef SIMDJSON_LASX_BASE_H
#define SIMDJSON_LASX_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
/**
 * Implementation for LASX.
 */
namespace lasx {

class implementation;

namespace {
namespace simd {
template <typename T> struct simd8;
template <typename T> struct simd8x64;
} // namespace simd
} // unnamed namespace

} // namespace lasx
} // namespace simdjson

#endif // SIMDJSON_LASX_BASE_H
