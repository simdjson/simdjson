#ifndef SIMDJSON_ARCHITECTURE_H
#define SIMDJSON_ARCHITECTURE_H

namespace simdjson {

// Represents the minimal architecture that would support an implementation
enum class architecture {
  UNSUPPORTED,
  WESTMERE,
  HASWELL,
  ARM64,
// TODO remove 'native' in favor of runtime dispatch?
// the 'native' enum class value should point at a good default on the current
// machine
#ifdef IS_X86_64
  NATIVE = WESTMERE
#elif defined(IS_ARM64)
  NATIVE = ARM64
#endif
};

architecture find_best_supported_architecture();
architecture parse_architecture(char *arch_name);

// backcompat
using Architecture = architecture;

} // namespace simdjson

#endif // SIMDJSON_ARCHITECTURE_H
