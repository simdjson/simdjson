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

// C++ 26
#if !defined(SIMDJSON_CPLUSPLUS26) && (SIMDJSON_CPLUSPLUS >= 202402L) // update when the standard is finalized
#define SIMDJSON_CPLUSPLUS26 1
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

#ifndef SIMDJSON_CONSTEXPR_LAMBDA
#if SIMDJSON_CPLUSPLUS17
#define SIMDJSON_CONSTEXPR_LAMBDA constexpr
#else
#define SIMDJSON_CONSTEXPR_LAMBDA
#endif
#endif



#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

// The current specification is unclear on how we detect
// static reflection, both __cpp_lib_reflection and
// __cpp_impl_reflection are proposed in the draft specification.
// For now, we disable static reflect by default. It must be
// specified at compiler time.
#ifndef SIMDJSON_STATIC_REFLECTION
#define SIMDJSON_STATIC_REFLECTION 0 // disabled by default.
#endif

#if defined(__apple_build_version__)
#if __apple_build_version__ < 14000000
#define SIMDJSON_CONCEPT_DISABLED 1 // apple-clang/13 doesn't support std::convertible_to
#endif
#endif

#if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 201911L
#include <ranges>
#define SIMDJSON_SUPPORTS_RANGES 1
#else
#define SIMDJSON_SUPPORTS_RANGES 0
#endif

#if defined(__cpp_concepts) && !defined(SIMDJSON_CONCEPT_DISABLED)
#if __cpp_concepts >= 201907L
#include <utility>
#define SIMDJSON_SUPPORTS_CONCEPTS 1
#else
#define SIMDJSON_SUPPORTS_CONCEPTS 0
#endif
#else // defined(__cpp_concepts) && !defined(SIMDJSON_CONCEPT_DISABLED)
#define SIMDJSON_SUPPORTS_CONCEPTS 0
#endif // defined(__cpp_concepts) && !defined(SIMDJSON_CONCEPT_DISABLED)

// copy SIMDJSON_SUPPORTS_CONCEPTS to SIMDJSON_SUPPORTS_DESERIALIZATION.
#if SIMDJSON_SUPPORTS_CONCEPTS
#define SIMDJSON_SUPPORTS_DESERIALIZATION 1
#else
#define SIMDJSON_SUPPORTS_DESERIALIZATION 0
#endif


#if !defined(SIMDJSON_CONSTEVAL)
#if defined(__cpp_consteval) && __cpp_consteval >= 201811L && defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907L
#define SIMDJSON_CONSTEVAL 1
#else
#define SIMDJSON_CONSTEVAL 0
#endif // defined(__cpp_consteval) && __cpp_consteval >= 201811L && defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907L
#endif // !defined(SIMDJSON_CONSTEVAL)

#endif // SIMDJSON_COMPILER_CHECK_H
