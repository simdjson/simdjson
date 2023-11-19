#ifndef SIMDJSON_PPC64_BASE_H
#define SIMDJSON_PPC64_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
/**
 * Implementation for ALTIVEC (PPC64).
 */
namespace ppc64 {

class implementation;

namespace simd {

template <typename T> struct simd8;
template <> struct simd8<bool>;
template <> struct simd8<uint8_t>;
template <typename T> struct simd8x64;

} // namespace simd

} // namespace ppc64
} // namespace simdjson

#endif // SIMDJSON_PPC64_BASE_H
