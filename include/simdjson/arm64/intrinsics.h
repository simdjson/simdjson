#ifndef SIMDJSON_ARM64_INTRINSICS_H
#define SIMDJSON_ARM64_INTRINSICS_H

// This should be the correct header whether
// you use visual studio or other compilers.
#include <arm_neon.h>

static_assert(sizeof(uint8x16_t) <= simdjson::SIMDJSON_PADDING, "insufficient padding for arm64");

#endif //  SIMDJSON_ARM64_INTRINSICS_H
