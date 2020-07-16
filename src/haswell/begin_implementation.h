#include "simdjson.h"

#include "haswell/implementation.h"
#include "haswell/intrinsics.h" // Generally need to be included outside SIMDJSON_TARGET_REGION

#define SIMDJSON_IMPLEMENTATION haswell
SIMDJSON_TARGET_REGION("avx2,bmi,pclmul,lzcnt")

#include "haswell/bitmanipulation.h"
#include "haswell/bitmask.h"
#include "haswell/simd.h"

