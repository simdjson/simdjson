#ifndef SIMDJSON_BUILTIN_BASE_H
#define SIMDJSON_BUILTIN_BASE_H

#include "simdjson/base.h"
#include "simdjson/implementation_detection.h"

namespace simdjson {
#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(arm64)
  namespace arm64 {}
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback)
  namespace fallback {}
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(haswell)
  namespace haswell {}
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(icelake)
  namespace icelake {}
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(ppc64)
  namespace ppc64 {}
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(westmere)
  namespace westmere {}
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(lsx)
  namespace lsx {}
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(lasx)
  namespace lasx {}
#else
#error Unknown SIMDJSON_BUILTIN_IMPLEMENTATION
#endif

  /**
   * Represents the best statically linked simdjson implementation that can be used by the compiling
   * program.
   *
   * Detects what options the program is compiled against, and picks the minimum implementation that
   * will work on any computer that can run the program. For example, if you compile with g++
   * -march=westmere, it will pick the westmere implementation. The haswell implementation will
   * still be available, and can be selected at runtime, but the builtin implementation (and any
   * code that uses it) will use westmere.
   */
  namespace builtin = SIMDJSON_BUILTIN_IMPLEMENTATION;
} // namespace simdjson

#endif // SIMDJSON_BUILTIN_BASE_H
