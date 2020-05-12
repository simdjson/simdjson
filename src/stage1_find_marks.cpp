#if SIMDJSON_IMPLEMENTATION_ARM64
#include "arm64/stage1.h"
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
#include "fallback/stage1.h"
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "haswell/stage1.h"
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
#include "westmere/stage1.h"
#endif
