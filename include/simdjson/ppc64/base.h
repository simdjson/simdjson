#ifndef SIMDJSON_PPC64_BASE_H
#define SIMDJSON_PPC64_BASE_H

#ifndef SIMDJSON_AMALGAMATED
#include "simdjson/base.h"
#endif // SIMDJSON_AMALGAMATED

namespace simdjson {
/**
 * Implementation for ALTIVEC (PPC64).
 */
namespace ppc64 {

class implementation;

namespace {
namespace simd {
template <typename T> struct simd8;
template <typename T> struct simd8x64;
} // namespace simd
} // unnamed namespace

} // namespace ppc64
} // namespace simdjson

#ifndef SIMDJSON_AMALGAMATED
// If we're editing one of the files in this directory, begin the implementation!
#ifndef SIMDJSON_IMPLEMENTATION
#include "simdjson/ppc64/begin.h"
#endif
#endif // SIMDJSON_AMALGAMATED

#endif // SIMDJSON_PPC64_BASE_H
