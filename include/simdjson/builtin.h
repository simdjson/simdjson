#ifndef SIMDJSON_BUILTIN_H
#define SIMDJSON_BUILTIN_H

#include "simdjson/portability.h"

#ifndef SIMDJSON_BUILTIN_IMPLEMENTATION
#if SIMDJSON_CAN_ALWAYS_RUN_HASWELL
#define SIMDJSON_BUILTIN_IMPLEMENTATION haswell
#elif SIMDJSON_CAN_ALWAYS_RUN_WESTMERE
#define SIMDJSON_BUILTIN_IMPLEMENTATION westmere
#elif SIMDJSON_CAN_ALWAYS_RUN_ARM64
#define SIMDJSON_BUILTIN_IMPLEMENTATION arm64
#elif SIMDJSON_CAN_ALWAYS_RUN_FALLBACK
#define SIMDJSON_BUILTIN_IMPLEMENTATION fallback
#else
#error "All possible implementations (including fallback) have been disabled! simdjson will not run."
#endif
#endif // SIMDJSON_BUILTIN_IMPLEMENTATION

namespace simdjson {
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
  /**
   * Function which returns a pointer to an implementation matching the "builtin" implementation.
   * The builtin implementation is the best statically linked simdjson implementation that can be used by the compiling
   * program. If you compile with g++ -march=haswell, this will return the haswell implementation.
   * It is handy to be able to check what builtin was used: builtin_implementation()->name().
   */
  const implementation * builtin_implementation();
} // namespace simdjson

#endif // SIMDJSON_BUILTIN_H
