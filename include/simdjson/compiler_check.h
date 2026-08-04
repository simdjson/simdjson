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
// While C++26 is a working draft, compilers report 202400L in C++26 mode
// (both GCC 16 and Clang 21 do). Update when the standard is finalized.
#if !defined(SIMDJSON_CPLUSPLUS26) && (SIMDJSON_CPLUSPLUS >= 202400L)
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

// Static reflection.
//
// The reflection-based APIs (simdjson::to, document::get<T>, the builder,
// compile-time JSON, annotations) need considerably more than the reflection
// operator. We turn them on only when the compiler advertises all of:
//
//   P2996 reflection (^^, splicers, <meta>)  __cpp_impl_reflection,
//                                            __cpp_lib_reflection
//   P1306 expansion statements (template for) __cpp_expansion_statements
//   P3491 std::define_static_string / _array  __cpp_lib_define_static
//
// Two further features we rely on have, as of this writing, no feature-test
// macro of their own, so they cannot be checked directly:
//
//   P3394 annotations ([[=x]], std::meta::annotations_of) -- used for
//         simdjson::rename and simdjson::skip.
//   P3289 consteval blocks (consteval { ... }) -- used by compile_time_json.
//
// Every implementation that defines the four macros above also implements
// those two, so requiring the four is sufficient in practice. If that ever
// stops being true, define SIMDJSON_STATIC_REFLECTION=0 to opt out.
//
// SIMDJSON_STATIC_REFLECTION may always be defined by the user (or by the
// build system) to 0 or 1 to override the detection.
//
// Note that C++26 mode alone is not enough: GCC 16 requires -freflection,
// and only then does it define __cpp_impl_reflection.
#ifndef SIMDJSON_STATIC_REFLECTION
#if defined(SIMDJSON_CPLUSPLUS26) &&                                           \
    defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202506L &&      \
    defined(__cpp_lib_reflection) && __cpp_lib_reflection >= 202506L &&        \
    defined(__cpp_expansion_statements) &&                                     \
        __cpp_expansion_statements >= 202506L &&                               \
    defined(__cpp_lib_define_static) && __cpp_lib_define_static >= 202506L
// __cpp_lib_reflection is the feature-test macro for <meta>, so there is no
// need for a separate __has_include check (which would have to be guarded for
// compilers that lack __has_include).
#define SIMDJSON_STATIC_REFLECTION 1
#else
#define SIMDJSON_STATIC_REFLECTION 0
#endif
#endif // SIMDJSON_STATIC_REFLECTION

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
