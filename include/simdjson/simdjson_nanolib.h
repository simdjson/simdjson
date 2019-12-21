#ifndef _SIMDJSON_INC_NANOLIB_
#define _SIMDJSON_INC_NANOLIB_

namespace simdjson {
    // compile time string comparison
    // C++11 or better
constexpr bool strings_equal(char const *lhs, char const *rhs) {
  while (*lhs || *rhs)
    if (*lhs++ != *rhs++)
      return false;
  return true;
}
} // simdjson

#endif // !_SIMDJSON_INC_NANOLIB_
