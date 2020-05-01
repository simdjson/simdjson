#ifndef SIMDJSON_COMPILER_CHECK_H
#define SIMDJSON_COMPILER_CHECK_H

#ifndef __cplusplus
#error simdjson requires a C++ compiler
#endif

#ifndef SIMDJSON_CPLUSPLUS
// MSVC now correctly reports __cplusplus
// https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
#if !defined(__cplusplus) && defined(_MSVC_LANG) && !defined(__clang__)
#define SIMDJSON_CPLUSPLUS (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
#else
#define SIMDJSON_CPLUSPLUS __cplusplus
#endif
#endif

// C++ 20
#if !defined(SIMDJSON_CPLUSPLUS20) && (SIMDJSON_CPLUSPLUS > 201703L)
#define SIMDJSON_CPLUSPLUS20 1
#endif

// C++ 17
#if !defined(SIMDJSON_CPLUSPLUS17) && (SIMDJSON_CPLUSPLUS >= 201703L)
#define SIMDJSON_CPLUSPLUS17 1
#endif

// C++ 14
#if !defined(SIMDJSON_CPLUSPLUS14) && (SIMDJSON_CPLUSPLUS >= 201402L)
#define SIMDJSON_CPLUSPLUS14 1
#endif

// C++ 11
#if !defined(SIMDJSON_CPLUSPLUS11) && (SIMDJSON_CPLUSPLUS >= 201103L)
#define SIMDJSON_CPLUSPLUS11 1
#endif

#ifndef SIMDJSON_CPLUSPLUS11
#error simdjson requires a compiler compliant with the C++11 standard
#endif

// We check that we are under cl.exe, with C++20 support
#if defined(_MSC_VER) && !defined(__clang__) && !defined(SIMDJSON_CPLUSPLUS20)
 // https://en.wikipedia.org/wiki/C_alternative_tokens
 // This header should have no effect, except
 // under Visual Studio.
 #include <ciso646>
// Microsoft states:
// Because this compatibility header defines names that are
// keywords in C++, including it has no effect. The <iso646.h>
// header is deprecated in C++. The <ciso646> header is removed
// in the draft C++20 standard.
// So we anticipate that Visual Studio might remove the header
// as part of the C++20 standard compliance.
// https://docs.microsoft.com/en-us/cpp/standard-library/ciso646?view=vs-2019
#endif


#endif // SIMDJSON_COMPILER_CHECK_H
