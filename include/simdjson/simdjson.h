#ifndef SIMDJSON_SIMDJSON_H
#define SIMDJSON_SIMDJSON_H

#include <string>
#include "simdjson_nanolib.h"
#include "isadetection.h"

namespace simdjson {
// Represents the minimal architecture that would support an implementation
enum class Architecture : unsigned short {
  UNSUPPORTED = 0,
  WESTMERE = 1,
  HASWELL = 2,
  ARM64 = 3,
// TODO remove 'native' in favor of runtime dispatch?
// the 'native' enum class value should point at a good default on the current
// machine
#ifdef IS_X86_64
  NATIVE = WESTMERE
#elif defined(IS_ARM64)
  NATIVE = ARM64
#endif
};

inline Architecture find_best_supported_architecture() {
  constexpr uint32_t haswell_flags =
      instruction_set::AVX2 | instruction_set::PCLMULQDQ |
      instruction_set::BMI1 | instruction_set::BMI2;
  constexpr uint32_t westmere_flags =
      instruction_set::SSE42 | instruction_set::PCLMULQDQ;

  uint32_t supports = detect_supported_architectures();
  // Order from best to worst (within architecture)
  if ((haswell_flags & supports) == haswell_flags)
    return Architecture::HASWELL;
  if ((westmere_flags & supports) == westmere_flags)
    return Architecture::WESTMERE;
  if (supports & instruction_set::NEON)
    return Architecture::ARM64;

  return Architecture::UNSUPPORTED;
}

/*
NOTE: for compile time result, argument has to be compile time value
*/
constexpr inline Architecture parse_architecture(char const *architecture) {
  if (strings_equal(architecture, "HASWELL")) {
    return Architecture::HASWELL;
  }
  if (strings_equal(architecture, "WESTMERE")) {
    return Architecture::WESTMERE;
  }
  if (strings_equal(architecture, "ARM64")) {
    return Architecture::ARM64;
  }
  return Architecture::UNSUPPORTED;
}

/// <summary>
/// call this once upon entering main() , if it returns false
/// there is no point of proceeding
/// </summary>
/// <returns>bool</returns>
inline bool is_cpu_supported() {
  if (Architecture::UNSUPPORTED == find_best_supported_architecture()) {
    return false;
  }
  return true;
}

/*
this should be enum class ErrorValues {};
*/
enum ErrorValues {
  SUCCESS = 0,
  SUCCESS_AND_HAS_MORE, //No errors and buffer still has more data
  CAPACITY,    // This ParsedJson can't support a document that big
  MEMALLOC,    // Error allocating memory, most likely out of memory
  TAPE_ERROR,  // Something went wrong while writing to the tape (stage 2), this
               // is a generic error
  DEPTH_ERROR, // Your document exceeds the user-specified depth limitation
  STRING_ERROR,    // Problem while parsing a string
  T_ATOM_ERROR,    // Problem while parsing an atom starting with the letter 't'
  F_ATOM_ERROR,    // Problem while parsing an atom starting with the letter 'f'
  N_ATOM_ERROR,    // Problem while parsing an atom starting with the letter 'n'
  NUMBER_ERROR,    // Problem while parsing a number
  UTF8_ERROR,      // the input is not valid UTF-8
  UNITIALIZED,     // unknown error, or uninitialized document
  EMPTY,           // no structural document found
  UNESCAPED_CHARS, // found unescaped characters in a string.
  UNCLOSED_STRING, // missing quote at the end
  UNEXPECTED_ERROR // indicative of a bug in simdjson
};

/*
this should be 
char const *  error_message( ErrorValues );
*/
const std::string &error_message(const int);
} // namespace simdjson
#endif // SIMDJSON_SIMDJSON_H
