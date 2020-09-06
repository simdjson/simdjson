#define SIMDJSON_IMPLEMENTATION haswell
#define SIMDJSON_TARGET_IMPLEMENTATION SIMDJSON_TARGET_REGION("avx2,bmi,pclmul,lzcnt")
#define SIMDJSON_UNTARGET_IMPLEMENTATION SIMDJSON_UNTARGET_REGION

#include "haswell/implementation.h"
#include "haswell/intrinsics.h" // Generally need to be included outside SIMDJSON_TARGET_REGION

SIMDJSON_TARGET_IMPLEMENTATION

#include "haswell/bitmanipulation.h"
#include "haswell/bitmask.h"
#include "haswell/simd.h"

