#ifndef SIMDJSON_SIMDJSON_H
#define SIMDJSON_SIMDJSON_H

#include <string>
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
constexpr auto arch_name_ = parse_architecture(Architecture::ARM64) ;
*/
constexpr char const * architecture_name (Architecture code_) noexcept {
  // C++11 -- single return statement
  return (
      code_ == Architecture::ARM64
          ? "ARM64"
          : (code_ == Architecture::HASWELL
                 ? "HASWELL"
                 : (code_ == Architecture::WESTMERE ? "WESTMERE" : "UNSUPPORTED")));
}

/*
Compile time Architecture code (C++11). Example.

// user defined literal type
// leave all the default scafolding to the compiler
struct parser_type final
{
    const Architecture code_ {} ;
    // secret sauce is constexpr constructor
    constexpr parser_type( Architecture a_)  : code_(a_) {}
    constexpr Architecture arch_code () const noexcept { return code_ ; }
    constexpr char const * arch_name () const noexcept { return architecture_name(code_) ; }
} ;

// using the above requires x_ to be template argument
template<  Architecture x_ >
void test_parser() 
{
    constexpr parser_type amd_parser{ x_ } ;
    constexpr auto code = amd_parser.arch_code();
    constexpr auto name = amd_parser.arch_name();

    printf("\nFor architecture code %d, names is %s", code, name );
}

*/
// a literal type holding the simdjson::Architecture code
// one type --> many codes
struct compile_time_architecture_code {
  const Architecture arch_{};
  constexpr compile_time_architecture_code(Architecture x_) : arch_(x_) {}
  constexpr Architecture code() const noexcept { return arch_; }
  constexpr char const *name() const noexcept {
    return architecture_name(arch_) ;
  }
};

// long descriptive name is important
// shorcut is comfortable 
// when in doubt what is simdjson::CTA 
// one can always use IDE to find out
    using CTA = compile_time_architecture_code;

/*
-----------------------------------------------------------------------
simdjson::Architecture will not change at runtime.  So, one does not have to check 
for it repeatedly, from constructors for example.

If code_ arg is compile time one can make compile time CTA
*/
    constexpr CTA
        make_cta(Architecture code_) {
  // C++11 -- single return statement
  return (code_ == Architecture::ARM64
              ? CTA(Architecture::ARM64)
              : (code_ == Architecture::HASWELL
                     ? CTA(Architecture::HASWELL)
                     : (code_ == Architecture::WESTMERE
                            ? CTA(Architecture::WESTMERE)
                            : CTA(Architecture::UNSUPPORTED))));
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
