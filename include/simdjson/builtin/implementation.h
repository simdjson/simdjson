#ifndef SIMDJSON_BUILTIN_IMPLEMENTATION_H
#define SIMDJSON_BUILTIN_IMPLEMENTATION_H

#include "simdjson/builtin/base.h"

#include "simdjson/generic/dependencies.h"

#define SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(arm64)
#include "simdjson/arm64/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback)
#include "simdjson/fallback/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(haswell)
#include "simdjson/haswell/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(icelake)
#include "simdjson/icelake/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(ppc64)
#include "simdjson/ppc64/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(westmere)
#include "simdjson/westmere/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(lsx)
#include "simdjson/lsx/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(lasx)
#include "simdjson/lasx/implementation.h"
#else
#error Unknown SIMDJSON_BUILTIN_IMPLEMENTATION
#endif

#undef SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
  /**
   * Function which returns a pointer to an implementation matching the "builtin" implementation.
   * The builtin implementation is the best statically linked simdjson implementation that can be used by the compiling
   * program. If you compile with g++ -march=haswell, this will return the haswell implementation.
   * It is handy to be able to check what builtin was used: builtin_implementation()->name().
   */
  const implementation * builtin_implementation();
} // namespace simdjson

#endif // SIMDJSON_BUILTIN_IMPLEMENTATION_H