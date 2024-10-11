#define SIMDJSON_IMPLEMENTATION haswell

#include "simdjson/haswell/base.h"
#include "simdjson/haswell/intrinsics.h"

#if !SIMDJSON_CAN_ALWAYS_RUN_HASWELL
// We enable bmi2 only if LLVM/clang is used, because GCC may not
// make good use of it. See https://github.com/simdjson/simdjson/pull/2243
#if defined(__clang__)
SIMDJSON_TARGET_REGION("avx2,bmi,bmi2,pclmul,lzcnt,popcnt")
#else
SIMDJSON_TARGET_REGION("avx2,bmi,pclmul,lzcnt,popcnt")
#endif
#endif

#include "simdjson/haswell/bitmanipulation.h"
#include "simdjson/haswell/bitmask.h"
#include "simdjson/haswell/numberparsing_defs.h"
#include "simdjson/haswell/simd.h"
#include "simdjson/haswell/stringparsing_defs.h"
