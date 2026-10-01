#ifndef SIMDJSON_ARM64_INTRINSICS_H
#define SIMDJSON_ARM64_INTRINSICS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/arm64/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// This should be the correct header whether
// you use visual studio or other compilers.
#include <arm_neon.h>

#if defined(__GNUC__) && defined(__ARM_FEATURE_SVE2) && \
    defined(__ARM_NEON_SVE_BRIDGE)
#define SIMDJSON_ARM64_SVE2_MATCH
#include <arm_sve.h>
#include <arm_neon_sve_bridge.h>
#endif

static_assert(sizeof(uint8x16_t) <= simdjson::SIMDJSON_PADDING, "insufficient padding for arm64");

#endif //  SIMDJSON_ARM64_INTRINSICS_H
