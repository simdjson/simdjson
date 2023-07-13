#include "simdjson/builtin.h"

#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(arm64)
#include "simdjson/arm64/end.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback)
#include "simdjson/fallback/end.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(haswell)
#include "simdjson/haswell/end.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(icelake)
#include "simdjson/icelake/end.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(ppc64)
#include "simdjson/ppc64/end.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(westmere)
#include "simdjson/westmere/end.h"
#else
#error Unknown SIMDJSON_BUILTIN_IMPLEMENTATION
#endif