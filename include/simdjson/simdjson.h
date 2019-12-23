#ifndef SIMDJSON_SIMDJSON_H
#define SIMDJSON_SIMDJSON_H

// source: github.com/martinmoene
// C++ language version detection (C++20 is speculative):
// Note: VC14.0/1900 (VS2015) lacks too much from C++14.

#ifndef SIMDJSON_CPLUSPLUS
#if defined(_MSVC_LANG) && !defined(__clang__)
#define SIMDJSON_CPLUSPLUS (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
#else
#define SIMDJSON_CPLUSPLUS __cplusplus
#endif
#endif

#define SIMDJSON_CPP98_OR_GREATER (SIMDJSON_CPLUSPLUS >= 199711L)
#define SIMDJSON_CPP11_OR_GREATER (SIMDJSON_CPLUSPLUS >= 201103L)
#define SIMDJSON_CPP14_OR_GREATER (SIMDJSON_CPLUSPLUS >= 201402L)
#define SIMDJSON_CPP17_OR_GREATER (SIMDJSON_CPLUSPLUS >= 201703L)
#define SIMDJSON_CPP20_OR_GREATER (SIMDJSON_CPLUSPLUS >= 202000L)

#include <string>

namespace simdjson {
// Represents the minimal architecture that would support an implementation
enum class Architecture {
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

Architecture find_best_supported_architecture();
Architecture parse_architecture(char *architecture);

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
const std::string &error_message(const int);
} // namespace simdjson
#endif // SIMDJSON_SIMDJSON_H
