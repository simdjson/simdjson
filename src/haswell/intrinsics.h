#ifndef SIMDJSON_HASWELL_INTRINSICS_H
#define SIMDJSON_HASWELL_INTRINSICS_H

#include "simdjson.h"

#ifdef _MSC_VER
#include <intrin.h> // visual studio
#else
#include <x86intrin.h> // elsewhere
#endif // _MSC_VER

#endif // SIMDJSON_HASWELL_INTRINSICS_H
