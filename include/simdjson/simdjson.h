#ifndef SIMDJSON_SIMDJSON_H
#define SIMDJSON_SIMDJSON_H

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
