#ifndef SIMDJSON_COMPILER_CHECK_H
#define SIMDJSON_COMPILER_CHECK_H

#ifndef __cplusplus
#error simdjson requires a C++ compiler
#endif

#ifndef SIMDJSON_CPLUSPLUS
#if defined(_MSVC_LANG) && !defined(__clang__)
#define SIMDJSON_CPLUSPLUS (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
#else
#define SIMDJSON_CPLUSPLUS __cplusplus
#endif
#endif

// Backfill std::string_view using nonstd::string_view on C++11
#ifndef SIMDJSON_INCLUDE_NONSTD_STRING_VIEW
#define SIMDJSON_INCLUDE_NONSTD_STRING_VIEW 1
#endif // SIMDJSON_INCLUDE_NONSTD_STRING_VIEW
#if (SIMDJSON_CPLUSPLUS < 201703L and SIMDJSON_INCLUDE_NONSTD_STRING_VIEW)
// #error simdjson requires a compiler compliant with the C++17 standard
#include "nonstd/string_view.hpp"
namespace std {
  using string_view=nonstd::string_view;
}
#endif

#endif // SIMDJSON_COMPILER_CHECK_H
