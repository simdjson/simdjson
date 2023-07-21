#define SIMDJSON_IMPLEMENTATION haswell

#include "simdjson/haswell/base.h"
#include "simdjson/haswell/intrinsics.h"

#if !SIMDJSON_CAN_ALWAYS_RUN_HASWELL
SIMDJSON_TARGET_REGION("avx2,bmi,pclmul,lzcnt,popcnt")
#endif

#include "simdjson/haswell/bitmanipulation.h"
#include "simdjson/haswell/bitmask.h"
#include "simdjson/haswell/numberparsing_defs.h"
#include "simdjson/haswell/simd.h"
#include "simdjson/haswell/stringparsing_defs.h"
