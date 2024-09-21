#ifndef SIMDJSON_COMMON_DEFS_H
#define SIMDJSON_COMMON_DEFS_H

#include <cassert>
#include "simdjson/compiler_check.h"
#include "simdjson/portability.h"

namespace simdjson {
namespace internal {
/**
 * @private
 * Our own implementation of the C++17 to_chars function.
 * Defined in src/to_chars
 */
char *to_chars(char *first, const char *last, double value);
/**
 * @private
 * A number parsing routine.
 * Defined in src/from_chars
 */
double from_chars(const char *first) noexcept;
double from_chars(const char *first, const char* end) noexcept;
}

#ifndef SIMDJSON_EXCEPTIONS
#if __cpp_exceptions
#define SIMDJSON_EXCEPTIONS 1
#else
#define SIMDJSON_EXCEPTIONS 0
#endif
#endif

} // namespace simdjson

#if defined(__GNUC__)
  // Marks a block with a name so that MCA analysis can see it.
  #define SIMDJSON_BEGIN_DEBUG_BLOCK(name) __asm volatile("# LLVM-MCA-BEGIN " #name);
  #define SIMDJSON_END_DEBUG_BLOCK(name) __asm volatile("# LLVM-MCA-END " #name);
  #define SIMDJSON_DEBUG_BLOCK(name, block) BEGIN_DEBUG_BLOCK(name); block; END_DEBUG_BLOCK(name);
#else
  #define SIMDJSON_BEGIN_DEBUG_BLOCK(name)
  #define SIMDJSON_END_DEBUG_BLOCK(name)
  #define SIMDJSON_DEBUG_BLOCK(name, block)
#endif

// Align to N-byte boundary
#define SIMDJSON_ROUNDUP_N(a, n) (((a) + ((n)-1)) & ~((n)-1))
#define SIMDJSON_ROUNDDOWN_N(a, n) ((a) & ~((n)-1))

#define SIMDJSON_ISALIGNED_N(ptr, n) (((uintptr_t)(ptr) & ((n)-1)) == 0)

#if SIMDJSON_REGULAR_VISUAL_STUDIO

  #define simdjson_really_inline __forceinline
  #define simdjson_never_inline __declspec(noinline)

  #define simdjson_unused
  #define simdjson_warn_unused

  #ifndef simdjson_likely
  #define simdjson_likely(x) x
  #endif
  #ifndef simdjson_unlikely
  #define simdjson_unlikely(x) x
  #endif

  #define SIMDJSON_PUSH_DISABLE_WARNINGS __pragma(warning( push ))
  #define SIMDJSON_PUSH_DISABLE_ALL_WARNINGS __pragma(warning( push, 0 ))
  #define SIMDJSON_DISABLE_VS_WARNING(WARNING_NUMBER) __pragma(warning( disable : WARNING_NUMBER ))
  // Get rid of Intellisense-only warnings (Code Analysis)
  // Though __has_include is C++17, it is supported in Visual Studio 2017 or better (_MSC_VER>=1910).
  #ifdef __has_include
  #if __has_include(<CppCoreCheck\Warnings.h>)
  #include <CppCoreCheck\Warnings.h>
  #define SIMDJSON_DISABLE_UNDESIRED_WARNINGS SIMDJSON_DISABLE_VS_WARNING(ALL_CPPCORECHECK_WARNINGS)
  #endif
  #endif

  #ifndef SIMDJSON_DISABLE_UNDESIRED_WARNINGS
  #define SIMDJSON_DISABLE_UNDESIRED_WARNINGS
  #endif

  #define SIMDJSON_DISABLE_DEPRECATED_WARNING SIMDJSON_DISABLE_VS_WARNING(4996)
  #define SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING
  #define SIMDJSON_POP_DISABLE_WARNINGS __pragma(warning( pop ))

  #define SIMDJSON_PUSH_DISABLE_UNUSED_WARNINGS
  #define SIMDJSON_POP_DISABLE_UNUSED_WARNINGS

#else // SIMDJSON_REGULAR_VISUAL_STUDIO

  #define simdjson_really_inline inline __attribute__((always_inline))
  #define simdjson_never_inline inline __attribute__((noinline))

  #define simdjson_unused __attribute__((unused))
  #define simdjson_warn_unused __attribute__((warn_unused_result))

  #ifndef simdjson_likely
  #define simdjson_likely(x) __builtin_expect(!!(x), 1)
  #endif
  #ifndef simdjson_unlikely
  #define simdjson_unlikely(x) __builtin_expect(!!(x), 0)
  #endif

  #define SIMDJSON_PUSH_DISABLE_WARNINGS _Pragma("GCC diagnostic push")
  // gcc doesn't seem to disable all warnings with all and extra, add warnings here as necessary
  // We do it separately for clang since it has different warnings.
  #ifdef __clang__
  // clang is missing -Wmaybe-uninitialized.
  #define SIMDJSON_PUSH_DISABLE_ALL_WARNINGS SIMDJSON_PUSH_DISABLE_WARNINGS \
    SIMDJSON_DISABLE_GCC_WARNING(-Weffc++) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wall) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wconversion) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wextra) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wattributes) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wimplicit-fallthrough) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wnon-virtual-dtor) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wreturn-type) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wshadow) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wunused-parameter) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wunused-variable)
  #else // __clang__
  #define SIMDJSON_PUSH_DISABLE_ALL_WARNINGS SIMDJSON_PUSH_DISABLE_WARNINGS \
    SIMDJSON_DISABLE_GCC_WARNING(-Weffc++) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wall) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wconversion) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wextra) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wattributes) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wimplicit-fallthrough) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wnon-virtual-dtor) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wreturn-type) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wshadow) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wunused-parameter) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wunused-variable) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wmaybe-uninitialized) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wformat-security)
  #endif // __clang__

  #define SIMDJSON_PRAGMA(P) _Pragma(#P)
  #define SIMDJSON_DISABLE_GCC_WARNING(WARNING) SIMDJSON_PRAGMA(GCC diagnostic ignored #WARNING)
  #if SIMDJSON_CLANG_VISUAL_STUDIO
  #define SIMDJSON_DISABLE_UNDESIRED_WARNINGS SIMDJSON_DISABLE_GCC_WARNING(-Wmicrosoft-include)
  #else
  #define SIMDJSON_DISABLE_UNDESIRED_WARNINGS
  #endif
  #define SIMDJSON_DISABLE_DEPRECATED_WARNING SIMDJSON_DISABLE_GCC_WARNING(-Wdeprecated-declarations)
  #define SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING SIMDJSON_DISABLE_GCC_WARNING(-Wstrict-overflow)
  #define SIMDJSON_POP_DISABLE_WARNINGS _Pragma("GCC diagnostic pop")

  #define SIMDJSON_PUSH_DISABLE_UNUSED_WARNINGS SIMDJSON_PUSH_DISABLE_WARNINGS \
    SIMDJSON_DISABLE_GCC_WARNING(-Wunused)
  #define SIMDJSON_POP_DISABLE_UNUSED_WARNINGS SIMDJSON_POP_DISABLE_WARNINGS



#endif // MSC_VER

#if defined(simdjson_inline)
  // Prefer the user's definition of simdjson_inline; don't define it ourselves.
#elif defined(__GNUC__) && !defined(__OPTIMIZE__)
  // If optimizations are disabled, forcing inlining can lead to significant
  // code bloat and high compile times. Don't use simdjson_really_inline for
  // unoptimized builds.
  #define simdjson_inline inline
#else
  // Force inlining for most simdjson functions.
  #define simdjson_inline simdjson_really_inline
#endif

#if SIMDJSON_VISUAL_STUDIO
    /**
     * Windows users need to do some extra work when building
     * or using a dynamic library (DLL). When building, we need
     * to set SIMDJSON_DLLIMPORTEXPORT to __declspec(dllexport).
     * When *using* the DLL, the user needs to set
     * SIMDJSON_DLLIMPORTEXPORT __declspec(dllimport).
     *
     * Static libraries not need require such work.
     *
     * It does not matter here whether you are using
     * the regular visual studio or clang under visual
     * studio, you still need to handle these issues.
     *
     * Non-Windows systems do not have this complexity.
     */
    #if SIMDJSON_BUILDING_WINDOWS_DYNAMIC_LIBRARY
    // We set SIMDJSON_BUILDING_WINDOWS_DYNAMIC_LIBRARY when we build a DLL under Windows.
    // It should never happen that both SIMDJSON_BUILDING_WINDOWS_DYNAMIC_LIBRARY and
    // SIMDJSON_USING_WINDOWS_DYNAMIC_LIBRARY are set.
    #define SIMDJSON_DLLIMPORTEXPORT __declspec(dllexport)
    #elif SIMDJSON_USING_WINDOWS_DYNAMIC_LIBRARY
    // Windows user who call a dynamic library should set SIMDJSON_USING_WINDOWS_DYNAMIC_LIBRARY to 1.
    #define SIMDJSON_DLLIMPORTEXPORT __declspec(dllimport)
    #else
    // We assume by default static linkage
    #define SIMDJSON_DLLIMPORTEXPORT
    #endif

/**
 * Workaround for the vcpkg package manager. Only vcpkg should
 * ever touch the next line. The SIMDJSON_USING_LIBRARY macro is otherwise unused.
 */
#if SIMDJSON_USING_LIBRARY
#define SIMDJSON_DLLIMPORTEXPORT __declspec(dllimport)
#endif
/**
 * End of workaround for the vcpkg package manager.
 */
#else
    #define SIMDJSON_DLLIMPORTEXPORT
#endif

// C++17 requires string_view.
#if SIMDJSON_CPLUSPLUS17
#define SIMDJSON_HAS_STRING_VIEW
#include <string_view> // by the standard, this has to be safe.
#endif

// This macro (__cpp_lib_string_view) has to be defined
// for C++17 and better, but if it is otherwise defined,
// we are going to assume that string_view is available
// even if we do not have C++17 support.
#ifdef __cpp_lib_string_view
#define SIMDJSON_HAS_STRING_VIEW
#endif

// Some systems have string_view even if we do not have C++17 support,
// and even if __cpp_lib_string_view is undefined, it is the case
// with Apple clang version 11.
// We must handle it. *This is important.*
#ifndef SIMDJSON_HAS_STRING_VIEW
#if defined __has_include
// do not combine the next #if with the previous one (unsafe)
#if __has_include (<string_view>)
// now it is safe to trigger the include
#include <string_view> // though the file is there, it does not follow that we got the implementation
#if defined(_LIBCPP_STRING_VIEW)
// Ah! So we under libc++ which under its Library Fundamentals Technical Specification, which preceded C++17,
// included string_view.
// This means that we have string_view *even though* we may not have C++17.
#define SIMDJSON_HAS_STRING_VIEW
#endif // _LIBCPP_STRING_VIEW
#endif // __has_include (<string_view>)
#endif // defined __has_include
#endif // def SIMDJSON_HAS_STRING_VIEW
// end of complicated but important routine to try to detect string_view.

//
// Backfill std::string_view using nonstd::string_view on systems where
// we expect that string_view is missing. Important: if we get this wrong,
// we will end up with two string_view definitions and potential trouble.
// That is why we work so hard above to avoid it.
//
#ifndef SIMDJSON_HAS_STRING_VIEW
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include "simdjson/nonstd/string_view.hpp"
SIMDJSON_POP_DISABLE_WARNINGS

namespace std {
  using string_view = nonstd::string_view;
}
#endif // SIMDJSON_HAS_STRING_VIEW
#undef SIMDJSON_HAS_STRING_VIEW // We are not going to need this macro anymore.

/// If EXPR is an error, returns it.
#define SIMDJSON_TRY(EXPR) { auto _err = (EXPR); if (_err) { return _err; } }

// Unless the programmer has already set SIMDJSON_DEVELOPMENT_CHECKS,
// we want to set it under debug builds. We detect a debug build
// under Visual Studio when the _DEBUG macro is set. Under the other
// compilers, we use the fact that they define __OPTIMIZE__ whenever
// they allow optimizations.
// It is possible that this could miss some cases where SIMDJSON_DEVELOPMENT_CHECKS
// is helpful, but the programmer can set the macro SIMDJSON_DEVELOPMENT_CHECKS.
// It could also wrongly set SIMDJSON_DEVELOPMENT_CHECKS (e.g., if the programmer
// sets _DEBUG in a release build under Visual Studio, or if some compiler fails to
// set the __OPTIMIZE__ macro).
#ifndef SIMDJSON_DEVELOPMENT_CHECKS
#ifdef _MSC_VER
// Visual Studio seems to set _DEBUG for debug builds.
#ifdef _DEBUG
#define SIMDJSON_DEVELOPMENT_CHECKS 1
#endif // _DEBUG
#else // _MSC_VER
// All other compilers appear to set __OPTIMIZE__ to a positive integer
// when the compiler is optimizing.
#ifndef __OPTIMIZE__
#define SIMDJSON_DEVELOPMENT_CHECKS 1
#endif // __OPTIMIZE__
#endif // _MSC_VER
#endif // SIMDJSON_DEVELOPMENT_CHECKS

// The SIMDJSON_CHECK_EOF macro is a feature flag for the "don't require padding"
// feature.

#if SIMDJSON_CPLUSPLUS17
// if we have C++, then fallthrough is a default attribute
# define simdjson_fallthrough [[fallthrough]]
// check if we have __attribute__ support
#elif defined(__has_attribute)
// check if we have the __fallthrough__ attribute
#if __has_attribute(__fallthrough__)
// we are good to go:
# define simdjson_fallthrough                    __attribute__((__fallthrough__))
#endif // __has_attribute(__fallthrough__)
#endif // SIMDJSON_CPLUSPLUS17
// on some systems, we simply do not have support for fallthrough, so use a default:
#ifndef simdjson_fallthrough
# define simdjson_fallthrough do {} while (0)  /* fallthrough */
#endif // simdjson_fallthrough

#if SIMDJSON_DEVELOPMENT_CHECKS
#define SIMDJSON_DEVELOPMENT_ASSERT(expr) do { assert ((expr)); } while (0)
#else
#define SIMDJSON_DEVELOPMENT_ASSERT(expr) do { } while (0)
#endif

#ifndef SIMDJSON_UTF8VALIDATION
#define SIMDJSON_UTF8VALIDATION 1
#endif

#ifdef __has_include
// How do we detect that a compiler supports vbmi2?
// For sure if the following header is found, we are ok?
#if __has_include(<avx512vbmi2intrin.h>)
#define SIMDJSON_COMPILER_SUPPORTS_VBMI2 1
#endif
#endif

#ifdef _MSC_VER
#if _MSC_VER >= 1920
// Visual Studio 2019 and up support VBMI2 under x64 even if the header
// avx512vbmi2intrin.h is not found.
#define SIMDJSON_COMPILER_SUPPORTS_VBMI2 1
#endif
#endif

// By default, we allow AVX512.
#ifndef SIMDJSON_AVX512_ALLOWED
#define SIMDJSON_AVX512_ALLOWED 1
#endif

#endif // SIMDJSON_COMMON_DEFS_H
