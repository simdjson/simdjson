#ifndef SIMDJSON_PPC64_INTRINSICS_H
#define SIMDJSON_PPC64_INTRINSICS_H

#include "simdjson.h"

// This should be the correct header whether
// you use visual studio or other compilers.
#include <altivec.h>

#if defined(bool)
#undef bool
#endif

#if defined(vector)
#undef vector
#endif

#endif //  SIMDJSON_PPC64_INTRINSICS_H
