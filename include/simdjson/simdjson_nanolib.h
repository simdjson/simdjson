#ifndef _SIMDJSON_INC_NANOLIB_
#define _SIMDJSON_INC_NANOLIB_

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
} // simdjson

#endif // !_SIMDJSON_INC_NANOLIB_
