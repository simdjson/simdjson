#define SIMDJSON_IMPLEMENTATION arm64
#define SIMDJSON_TARGET_IMPLEMENTATION
#define SIMDJSON_UNTARGET_IMPLEMENTATION

#include "arm64/implementation.h"
#include "arm64/intrinsics.h"
#include "arm64/bitmanipulation.h"
#include "arm64/bitmask.h"
#include "arm64/simd.h"
