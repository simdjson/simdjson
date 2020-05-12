#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

#include "error.cpp"
#include "implementation.cpp"

#if SIMDJSON_IMPLEMENTATION_ARM64
#include "arm64/stage1.cpp"
#include "arm64/stage2.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
#include "fallback/stage1.cpp"
#include "fallback/stage2.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "haswell/stage1.cpp"
#include "haswell/stage2.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
#include "westmere/stage1.cpp"
#include "westmere/stage2.cpp"
#endif

SIMDJSON_POP_DISABLE_WARNINGS
