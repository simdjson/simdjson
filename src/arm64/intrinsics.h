#ifndef SIMDJSON_ARM64_INTRINSICS_H
#define SIMDJSON_ARM64_INTRINSICS_H
#ifdef IS_ARM64

// This should be the correct header whether
// you use visual studio or other compilers.
#include <arm_neon.h>
#endif //   IS_ARM64
#endif //  SIMDJSON_ARM64_INTRINSICS_H
