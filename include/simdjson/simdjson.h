#ifndef  SIMDJSON_ERR_H
# define SIMDJSON_ERR_H

#include <string>

namespace simdjson {
enum class instruction_set {
  avx2,
  sse4_2,
  neon,
  none,
// the 'native' enum class value should point at a good default on the current machine
#ifdef __AVX2__
  native = avx2
#elif defined(__ARM_NEON)
  native = neon
#else
  // Let us assume that we have an old x64 processor, but one that has SSE (i.e., something
  // that came out in the second decade of the XXIst century.
  // It would be nicer to check explicitly, but there many not be a good way to do so
  // that is cross-platform.
  // Under Visual Studio, there is no way to check for SSE4.2 support at compile-time.
  native = sse4_2
#endif
};

enum errorValues {
  SUCCESS = 0,
  CAPACITY, // This ParsedJson can't support a document that big
  MEMALLOC, // Error allocating memory, most likely out of memory
  TAPE_ERROR, // Something went wrong while writing to the tape (stage 2), this is a generic error
  DEPTH_ERROR, // Your document exceeds the user-specified depth limitation
  STRING_ERROR, // Problem while parsing a string
  T_ATOM_ERROR, // Problem while parsing an atom starting with the letter 't'
  F_ATOM_ERROR, // Problem while parsing an atom starting with the letter 'f'
  N_ATOM_ERROR, // Problem while parsing an atom starting with the letter 'n'
  NUMBER_ERROR, // Problem while parsing a number
  UTF8_ERROR, // the input is not valid UTF-8
  UNITIALIZED, // unknown error, or uninitialized document
  EMPTY, // no structural document found
  UNESCAPED_CHARS, // found unescaped characters in a string.
  UNCLOSED_STRING, // missing quote at the end
  UNEXPECTED_ERROR // indicative of a bug in simdjson
};
const std::string& errorMsg(const int);
}
#endif
