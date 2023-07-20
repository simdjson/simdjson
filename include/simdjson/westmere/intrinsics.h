#ifndef SIMDJSON_WESTMERE_INTRINSICS_H
#define SIMDJSON_WESTMERE_INTRINSICS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/westmere/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_VISUAL_STUDIO
// under clang within visual studio, this will include <x86intrin.h>
#include <intrin.h> // visual studio or clang
#else
#include <x86intrin.h> // elsewhere
#endif // SIMDJSON_VISUAL_STUDIO


#if SIMDJSON_CLANG_VISUAL_STUDIO
/**
 * You are not supposed, normally, to include these
 * headers directly. Instead you should either include intrin.h
 * or x86intrin.h. However, when compiling with clang
 * under Windows (i.e., when _MSC_VER is set), these headers
 * only get included *if* the corresponding features are detected
 * from macros:
 */
#include <smmintrin.h>  // for _mm_alignr_epi8
#include <wmmintrin.h>  // for  _mm_clmulepi64_si128
#endif

static_assert(sizeof(__m128i) <= simdjson::SIMDJSON_PADDING, "insufficient padding for westmere");

#endif // SIMDJSON_WESTMERE_INTRINSICS_H
