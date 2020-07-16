#include "simdjson.h"
#include "arm64/implementation.h"
#include "arm64/intrinsics.h"

#define SIMDJSON_IMPLEMENTATION arm64

#include "arm64/bitmanipulation.h"
#include "arm64/bitmask.h"
#include "arm64/simd.h"
