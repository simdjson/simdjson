#if SIMDJSON_IMPLEMENTATION_ARM64
#include "arm64/stage1_find_marks.h"
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
#include "fallback/stage1_find_marks.h"
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "haswell/stage1_find_marks.h"
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
#include "westmere/stage1_find_marks.h"
#endif
