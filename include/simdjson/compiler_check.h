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

// C++ 23
#if !defined(SIMDJSON_CPLUSPLUS23) && (SIMDJSON_CPLUSPLUS >= 202302L)
#define SIMDJSON_CPLUSPLUS23 1
#endif

// C++ 20
#if !defined(SIMDJSON_CPLUSPLUS20) && (SIMDJSON_CPLUSPLUS >= 202002L)
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

#ifndef SIMDJSON_IF_CONSTEXPR
#if SIMDJSON_CPLUSPLUS17
#define SIMDJSON_IF_CONSTEXPR if constexpr
#else
#define SIMDJSON_IF_CONSTEXPR if
#endif
#endif

#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#if defined(__apple_build_version__)
#if __apple_build_version__ < 14000000
#define SIMDJSON_CONCEPT_DISABLED 1 // apple-clang/13 doesn't support std::convertible_to
#endif
#endif


#if defined(__cpp_concepts) && !defined(SIMDJSON_CONCEPT_DISABLED)
#if __cpp_concepts >= 201907L
#include <utility>
#define SIMDJSON_SUPPORTS_DESERIALIZATION 1
#else
#define SIMDJSON_SUPPORTS_DESERIALIZATION 0
#endif
#else // defined(__cpp_concepts) && !defined(SIMDJSON_CONCEPT_DISABLED)
#define SIMDJSON_SUPPORTS_DESERIALIZATION 0
#endif // defined(__cpp_concepts) && !defined(SIMDJSON_CONCEPT_DISABLED)

#endif // SIMDJSON_COMPILER_CHECK_H
