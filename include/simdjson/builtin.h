#ifndef SIMDJSON_BUILTIN_H
#define SIMDJSON_BUILTIN_H

#include "simdjson/implementation_detection.h"

#include "simdjson/builtin/implementation.h"

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
