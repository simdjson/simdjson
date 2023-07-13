#define SIMDJSON_IMPLEMENTATION icelake
#include "simdjson/icelake/base.h"
#include "simdjson/icelake/intrinsics.h"

#if !SIMDJSON_CAN_ALWAYS_RUN_ICELAKE
SIMDJSON_TARGET_REGION("avx512f,avx512dq,avx512cd,avx512bw,avx512vbmi,avx512vbmi2,avx512vl,avx2,bmi,pclmul,lzcnt,popcnt")
#endif

#include "simdjson/icelake/bitmanipulation.h"
#include "simdjson/icelake/bitmask.h"
#include "simdjson/icelake/simd.h"
#include "simdjson/icelake/stringparsing_defs.h"
#include "simdjson/icelake/numberparsing_defs.h"
