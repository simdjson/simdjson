#ifndef SIMDJSON_WESTMERE_INTRINSICS_H
#define SIMDJSON_WESTMERE_INTRINSICS_H

#ifdef SIMDJSON_VISUAL_STUDIO
// under clang within visual studio, this will include <x86intrin.h>
#include <intrin.h> // visual studio or clang
#else
#include <x86intrin.h> // elsewhere
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO

#endif // SIMDJSON_WESTMERE_INTRINSICS_H
