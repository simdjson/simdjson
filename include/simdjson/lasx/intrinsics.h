#ifndef SIMDJSON_LASX_INTRINSICS_H
#define SIMDJSON_LASX_INTRINSICS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/lasx/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// This should be the correct header whether
// you use visual studio or other compilers.
#include <lasxintrin.h>

static_assert(sizeof(__m256i) <= simdjson::SIMDJSON_PADDING, "insufficient padding for LoongArch ASX");

#endif //  SIMDJSON_LASX_INTRINSICS_H
