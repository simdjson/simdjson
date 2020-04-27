#ifndef SIMDJSON_HASWELL_INTRINSICS_H
#define SIMDJSON_HASWELL_INTRINSICS_H

#include "simdjson.h"

#ifdef _MSC_VER
// visual studio (or clang-cl)
#include <intrin.h>
#ifdef __clang__
#include <bmiintrin.h>
#include <lzcntintrin.h>
#include <immintrin.h>
#include <wmmintrin.h>
#endif
#else
#include <x86intrin.h> // elsewhere
#endif // _MSC_VER

#endif // SIMDJSON_HASWELL_INTRINSICS_H
