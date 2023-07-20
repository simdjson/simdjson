#ifndef SIMDJSON_ARM64_INTRINSICS_H
#define SIMDJSON_ARM64_INTRINSICS_H

#ifndef SIMDJSON_AMALGAMATED
#include "simdjson/arm64/base.h"
#endif // SIMDJSON_AMALGAMATED

// This should be the correct header whether
// you use visual studio or other compilers.
#include <arm_neon.h>

static_assert(sizeof(uint8x16_t) <= simdjson::SIMDJSON_PADDING, "insufficient padding for arm64");

#endif //  SIMDJSON_ARM64_INTRINSICS_H
