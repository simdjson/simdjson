#ifndef _SIMDJSON_INC_NANOLIB_
#define _SIMDJSON_INC_NANOLIB_

/*
Temporary shelter for new abstractions in the TBD state

|~/benchmark/json_parser.h constains and uses architecture_services
*/

namespace simdjson {
namespace nanolib {
//
// compile time native string handling -- C++11 or better
//
// NOTE: for compiler time, arguments have to be compile time values
// that is: literals or constexpr values
// for testing use https://godbolt.org/z/gGL95T
//
// NOTE: C++20 char8_t is a path peppered with a shards of glass, just don't go 
// there 
//
// NOTE: char16_t and char32_t are ok. if need them add them bellow 
//
// NOTE: WIN32 is UTF-16 aka wchar_t universe, WIN32 char API's are all translated to
// wchar_t
//
constexpr bool strings_equal(char const *lhs, char const *rhs) noexcept {
  while (*lhs || *rhs)
    if (*lhs++ != *rhs++)
      return false;
  return true;
}

constexpr bool strings_equal(wchar_t const *lhs, wchar_t const *rhs) noexcept {
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
/*
| ~ / benchmark / json_parser.h constains and uses architecture_services 
* /
 namespace architecture_services {
/*
constexpr auto arch_name_ = parse_architecture(Architecture::ARM64) ;
*/
constexpr char const *architecture_name(Architecture code_) noexcept {
  // C++11 -- single return statement
  return (code_ == Architecture::ARM64
              ? "ARM64"
              : (code_ == Architecture::HASWELL
                     ? "HASWELL"
                     : (code_ == Architecture::WESTMERE ? "WESTMERE"
                                                        : "UNSUPPORTED")));
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
    constexpr char const * arch_name () const noexcept { return
architecture_name(code_) ; } } ;

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
    return architecture_name(arch_);
  }
};

// long descriptive name is important
// shorcut is comfortable
// when in doubt what is simdjson::CTA
// one can always use IDE to find out
using CTA = compile_time_architecture_code;

/*
-----------------------------------------------------------------------
simdjson::Architecture will not change at runtime.  So, one does not have to
check for it repeatedly, from constructors for example.

If code_ arg is compile time one can make compile time CTA
*/
constexpr CTA make_cta(Architecture code_) {
  // C++11 -- single return statement
  return (code_ == Architecture::ARM64
              ? CTA(Architecture::ARM64)
              : (code_ == Architecture::HASWELL
                     ? CTA(Architecture::HASWELL)
                     : (code_ == Architecture::WESTMERE
                            ? CTA(Architecture::WESTMERE)
                            : CTA(Architecture::UNSUPPORTED))));
}
}
} // simdjson

#endif // !_SIMDJSON_INC_NANOLIB_
