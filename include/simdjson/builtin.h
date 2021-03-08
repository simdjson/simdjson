#ifndef SIMDJSON_BUILTIN_H
#define SIMDJSON_BUILTIN_H

#include "simdjson/implementations.h"

// Determine the best builtin implementation
#ifndef SIMDJSON_BUILTIN_IMPLEMENTATION
#if SIMDJSON_CAN_ALWAYS_RUN_HASWELL
#define SIMDJSON_BUILTIN_IMPLEMENTATION haswell
#elif SIMDJSON_CAN_ALWAYS_RUN_WESTMERE
#define SIMDJSON_BUILTIN_IMPLEMENTATION westmere
#elif SIMDJSON_CAN_ALWAYS_RUN_ARM64
#define SIMDJSON_BUILTIN_IMPLEMENTATION arm64
#elif SIMDJSON_CAN_ALWAYS_RUN_PPC64
#define SIMDJSON_BUILTIN_IMPLEMENTATION ppc64
#elif SIMDJSON_CAN_ALWAYS_RUN_FALLBACK
#define SIMDJSON_BUILTIN_IMPLEMENTATION fallback
#else
#error "All possible implementations (including fallback) have been disabled! simdjson will not run."
#endif
#endif // SIMDJSON_BUILTIN_IMPLEMENTATION

#define SIMDJSON_IMPLEMENTATION SIMDJSON_BUILTIN_IMPLEMENTATION

// ondemand is only compiled as part of the builtin implementation at present

// Interface declarations
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/ondemand.h"

// Inline definitions
#include "simdjson/generic/implementation_simdjson_result_base-inl.h"
#include "simdjson/generic/ondemand-inl.h"

#undef SIMDJSON_IMPLEMENTATION

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
   * @copydoc simdjson::SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand
   */
  namespace ondemand = SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand;
  /**
   * Function which returns a pointer to an implementation matching the "builtin" implementation.
   * The builtin implementation is the best statically linked simdjson implementation that can be used by the compiling
   * program. If you compile with g++ -march=haswell, this will return the haswell implementation.
   * It is handy to be able to check what builtin was used: builtin_implementation()->name().
   */
  const implementation * builtin_implementation();
} // namespace simdjson

#endif // SIMDJSON_BUILTIN_H
