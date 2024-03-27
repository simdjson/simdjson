#ifndef SIMDJSON_LSX_INTRINSICS_H
#define SIMDJSON_LSX_INTRINSICS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/lsx/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// This should be the correct header whether
// you use visual studio or other compilers.
#include <lsxintrin.h>

static_assert(sizeof(__m128i) <= simdjson::SIMDJSON_PADDING, "insufficient padding for LoongArch SX");

#endif //  SIMDJSON_LSX_INTRINSICS_H
