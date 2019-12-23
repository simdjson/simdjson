#ifndef SIMDJSON_SIMDJSON_H
#define SIMDJSON_SIMDJSON_H

// from MSVC STD LIB -- now open source
#if !defined(_SIMDJSON_HAS_CXX17) && !defined(_SIMDJSON_HAS_CXX20)
#if defined(_MSVC_LANG)
#define _SIMDJSON__STL_LANG _MSVC_LANG
#else // ^^^ use _MSVC_LANG / use __cplusplus vvv
#define _SIMDJSON__STL_LANG __cplusplus
#endif // ^^^ use __cplusplus ^^^

#if _SIMDJSON__STL_LANG > 201703L
#define _SIMDJSON_HAS_CXX17 1
#define _SIMDJSON_HAS_CXX20 1
#elif _SIMDJSON__STL_LANG > 201402L
#define _SIMDJSON_HAS_CXX17 1
#define _SIMDJSON_HAS_CXX20 0
#else // _SIMDJSON__STL_LANG <= 201402L
#define _SIMDJSON_HAS_CXX17 0
#define _SIMDJSON_HAS_CXX20 0
#endif // Use the value of _SIMDJSON__STL_LANG to define _SIMDJSON_HAS_CXX17 and
       // _SIMDJSON_HAS_CXX20

#undef _SIMDJSON__STL_LANG
#endif // !defined(_SIMDJSON_HAS_CXX17) && !defined(_SIMDJSON_HAS_CXX20)


#include "isadetection.h"

namespace simdjson {
  namespace nanolib {
  //
  // compile time native string handling -- C++11 or better
  //
  // NOTE: for compiler time, arguments have to be compile time values
  // that is: literals or constexpr values
  // for testing use https://godbolt.org/z/gGL95T
  //
  // NOTE: C++20 char8_t is a path peppered with a shards of glass, just don't
  // go there
  //
  // NOTE: char16_t and char32_t are ok. if need them add them bellow
  //
  // NOTE: WIN32 is UTF-16 aka wchar_t universe, WIN32 char API's are all
  // translated to wchar_t
  //
  constexpr bool strings_equal(char const *lhs, char const *rhs) noexcept {
    while (*lhs || *rhs)
      if (*lhs++ != *rhs++)
        return false;
    return true;
  }

  constexpr bool strings_equal(wchar_t const *lhs,
                               wchar_t const *rhs) noexcept {
    while (*lhs || *rhs)
      if (*lhs++ != *rhs++)
        return false;
    return true;
  }

  constexpr size_t ct_strlen(const char *ch) noexcept {
    size_t len = 0;
    while (ch[len])
      ++len;
    return len;
  }

  constexpr size_t ct_strlen(const wchar_t *ch) noexcept {
    size_t len = 0;
    while (ch[len])
      ++len;
    return len;
  }
  } // namespace nanolib
} // simdjson


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

/*
NOTE: for compile time result, argument has to be compile time value

constexpr Architecture arch_code_ = parse_architecture("ARM64") ;
*/
constexpr inline Architecture parse_architecture(char const *architecture) {
    
    using namespace simdjson::nanolib ;

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

/*
---------------------------------------------------------------------------
if this function could be compile time, many simdjson  API's could be too
*/
inline Architecture find_best_supported_architecture() {
  constexpr uint32_t haswell_flags =
      instruction_set::AVX2 | instruction_set::PCLMULQDQ |
      instruction_set::BMI1 | instruction_set::BMI2;
  constexpr uint32_t westmere_flags =
      instruction_set::SSE42 | instruction_set::PCLMULQDQ;

  // because of this call the whole of this function
  // can not be compile time
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
please fix!
*/
enum ErrorValues {
  SUCCESS = 0,
  SUCCESS_AND_HAS_MORE, // No errors and buffer still has more data
  CAPACITY,             // This ParsedJson can't support a document that big
  MEMALLOC,             // Error allocating memory, most likely out of memory
  TAPE_ERROR,  // Something went wrong while writing to the tape (stage 2), this
               // is a generic error
  DEPTH_ERROR, // Your document exceeds the user-specified depth limitation
  STRING_ERROR,    // Problem while parsing a string
  T_ATOM_ERROR,    // Problem while parsing an atom starting with the letter 't'
  F_ATOM_ERROR,    // Problem while parsing an atom starting with the letter 'f'
  N_ATOM_ERROR,    // Problem while parsing an atom starting with the letter 'n'
  NUMBER_ERROR,    // Problem while parsing a number
  UTF8_ERROR,      // the input is not valid UTF-8
  UNINITIALIZED,   // unknown error, or uninitialized document
  EMPTY,           // no structural document found
  UNESCAPED_CHARS, // found unescaped characters in a string.
  UNCLOSED_STRING, // missing quote at the end
  UNEXPECTED_ERROR // indicative of a bug in simdjson
};

namespace detail {

struct error_and_message final {
  /*simdjson::ErrorValues*/ int code;
  char const *message;
};

/*
inline var is C++17 feature, 
thus we must use static 
and static will grow the code
*/
static const error_and_message errors_and_messages[]{
    {/* ErrorValue:s: */ SUCCESS, "No errors"}, /* 0 */
    {/* ErrorValue:s: */ SUCCESS_AND_HAS_MORE,
     "No errors and buffer still has more data"},
    {/* ErrorValue:s: */ CAPACITY,
     "This ParsedJson can't support a document that big"},
    {/* ErrorValue:s: */ MEMALLOC,
     "Error allocating memory, we're most likely out of memory"},
    {/* ErrorValue:s: */ TAPE_ERROR,
     "Something went wrong while writing to the tape"},
    {/* ErrorValue:s: */ STRING_ERROR, "Problem while parsing a string"},
    {/* ErrorValue:s: */ T_ATOM_ERROR,
     "Problem while parsing an atom starting with the letter 't'"},
    {/* ErrorValue:s: */ F_ATOM_ERROR,
     "Problem while parsing an atom starting with the letter 'f'"},
    {/* ErrorValue:s: */ N_ATOM_ERROR,
     "Problem while parsing an atom starting with the letter 'n'"},
    {/* ErrorValue:s: */ NUMBER_ERROR, "Problem while parsing a number"},
    {/* ErrorValue:s: */ UTF8_ERROR, "The input is not valid UTF-8"},
    {/* ErrorValue:s: */ UNINITIALIZED, "Uninitialized"},
    {/* ErrorValue:s: */ EMPTY, "Empty: no JSON found"},
    {/* ErrorValue:s: */ UNESCAPED_CHARS,
     "Within strings, some characters must be escaped, we "
     "found unescaped characters"},
    {/* ErrorValue:s: */ UNCLOSED_STRING,
     "A string is opened, but never closed."},
    {/* ErrorValue:s: */ UNEXPECTED_ERROR,
     "Unexpected error, consider reporting this problem as "
     "you may have found a bug in simdjson"},
};
} // namespace detail

// returns a string matching the error code
inline char const *error_message(/*simdjson::ErrorValues*/ int err_code_) {
  // DANGER: hack ahead
  return detail::errors_and_messages[size_t(err_code_)].message;
}

} // namespace simdjson
#endif // SIMDJSON_SIMDJSON_H
