#ifndef SIMDJSON_PORTABILITY_H
#define SIMDJSON_PORTABILITY_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cfloat>
#include <cassert>
#ifndef _WIN32
// strcasecmp, strncasecmp
#include <strings.h>
#endif

#ifdef _MSC_VER
#define SIMDJSON_VISUAL_STUDIO 1
/**
 * We want to differentiate carefully between
 * clang under visual studio and regular visual
 * studio.
 *
 * Under clang for Windows, we enable:
 *  * target pragmas so that part and only part of the
 *     code gets compiled for advanced instructions.
 *
 */
#ifdef __clang__
// clang under visual studio
#define SIMDJSON_CLANG_VISUAL_STUDIO 1
#else
// just regular visual studio (best guess)
#define SIMDJSON_REGULAR_VISUAL_STUDIO 1
#endif // __clang__
#endif // _MSC_VER

#if defined(__x86_64__) || defined(_M_AMD64)
#define SIMDJSON_IS_X86_64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define SIMDJSON_IS_ARM64 1
#elif defined(__PPC64__) || defined(_M_PPC64)
#if defined(__ALTIVEC__)
#define SIMDJSON_IS_PPC64_VMX 1
#endif // defined(__ALTIVEC__)
#else
#define SIMDJSON_IS_32BITS 1

// We do not support 32-bit platforms, but it can be
// handy to identify them.
#if defined(_M_IX86) || defined(__i386__)
#define SIMDJSON_IS_X86_32BITS 1
#elif defined(__arm__) || defined(_M_ARM)
#define SIMDJSON_IS_ARM_32BITS 1
#elif defined(__PPC__) || defined(_M_PPC)
#define SIMDJSON_IS_PPC_32BITS 1
#endif

#endif // defined(__x86_64__) || defined(_M_AMD64)
#ifndef SIMDJSON_IS_32BITS
#define SIMDJSON_IS_32BITS 0
#endif

#if SIMDJSON_IS_32BITS
#ifndef SIMDJSON_NO_PORTABILITY_WARNING
#pragma message("The simdjson library is designed \
for 64-bit processors and it seems that you are not \
compiling for a known 64-bit platform. All fast kernels \
will be disabled and performance may be poor. Please \
use a 64-bit target such as x64, 64-bit ARM or 64-bit PPC.")
#endif // SIMDJSON_NO_PORTABILITY_WARNING
#endif // SIMDJSON_IS_32BITS

// this is almost standard?
#undef SIMDJSON_STRINGIFY_IMPLEMENTATION_
#undef SIMDJSON_STRINGIFY
#define SIMDJSON_STRINGIFY_IMPLEMENTATION_(a) #a
#define SIMDJSON_STRINGIFY(a) SIMDJSON_STRINGIFY_IMPLEMENTATION_(a)

// Our fast kernels require 64-bit systems.
//
// On 32-bit x86, we lack 64-bit popcnt, lzcnt, blsr instructions.
// Furthermore, the number of SIMD registers is reduced.
//
// On 32-bit ARM, we would have smaller registers.
//
// The simdjson users should still have the fallback kernel. It is
// slower, but it should run everywhere.

//
// Enable valid runtime implementations, and select SIMDJSON_BUILTIN_IMPLEMENTATION
//

// We are going to use runtime dispatch.
#if SIMDJSON_IS_X86_64
#ifdef __clang__
// clang does not have GCC push pop
// warning: clang attribute push can't be used within a namespace in clang up
// til 8.0 so SIMDJSON_TARGET_REGION and SIMDJSON_UNTARGET_REGION must be *outside* of a
// namespace.
#define SIMDJSON_TARGET_REGION(T)                                                       \
  _Pragma(SIMDJSON_STRINGIFY(                                                           \
      clang attribute push(__attribute__((target(T))), apply_to = function)))
#define SIMDJSON_UNTARGET_REGION _Pragma("clang attribute pop")
#elif defined(__GNUC__)
// GCC is easier
#define SIMDJSON_TARGET_REGION(T)                                                       \
  _Pragma("GCC push_options") _Pragma(SIMDJSON_STRINGIFY(GCC target(T)))
#define SIMDJSON_UNTARGET_REGION _Pragma("GCC pop_options")
#endif // clang then gcc

#endif // x86

// Default target region macros don't do anything.
#ifndef SIMDJSON_TARGET_REGION
#define SIMDJSON_TARGET_REGION(T)
#define SIMDJSON_UNTARGET_REGION
#endif

// Is threading enabled?
#if defined(_REENTRANT) || defined(_MT)
#ifndef SIMDJSON_THREADS_ENABLED
#define SIMDJSON_THREADS_ENABLED
#endif
#endif

// workaround for large stack sizes under -O0.
// https://github.com/simdjson/simdjson/issues/691
#ifdef __APPLE__
#ifndef __OPTIMIZE__
// Apple systems have small stack sizes in secondary threads.
// Lack of compiler optimization may generate high stack usage.
// Users may want to disable threads for safety, but only when
// in debug mode which we detect by the fact that the __OPTIMIZE__
// macro is not defined.
#undef SIMDJSON_THREADS_ENABLED
#endif
#endif


#if defined(__clang__)
#define SIMDJSON_NO_SANITIZE_UNDEFINED __attribute__((no_sanitize("undefined")))
#elif defined(__GNUC__)
#define SIMDJSON_NO_SANITIZE_UNDEFINED __attribute__((no_sanitize_undefined))
#else
#define SIMDJSON_NO_SANITIZE_UNDEFINED
#endif


#if defined(__clang__) || defined(__GNUC__)
#if defined(__has_feature)
#  if __has_feature(memory_sanitizer)
#define SIMDJSON_NO_SANITIZE_MEMORY __attribute__((no_sanitize("memory")))
#  endif // if __has_feature(memory_sanitizer)
#endif // defined(__has_feature)
#endif
// make sure it is defined as 'nothing' if it is unapplicable.
#ifndef SIMDJSON_NO_SANITIZE_MEMORY
#define SIMDJSON_NO_SANITIZE_MEMORY
#endif

#if SIMDJSON_VISUAL_STUDIO
// This is one case where we do not distinguish between
// regular visual studio and clang under visual studio.
// clang under Windows has _stricmp (like visual studio) but not strcasecmp (as clang normally has)
#define simdjson_strcasecmp _stricmp
#define simdjson_strncasecmp _strnicmp
#else
// The strcasecmp, strncasecmp, and strcasestr functions do not work with multibyte strings (e.g. UTF-8).
// So they are only useful for ASCII in our context.
// https://www.gnu.org/software/libunistring/manual/libunistring.html#char-_002a-strings
#define simdjson_strcasecmp strcasecmp
#define simdjson_strncasecmp strncasecmp
#endif

#ifdef NDEBUG

#if SIMDJSON_VISUAL_STUDIO
#define SIMDJSON_UNREACHABLE() __assume(0)
#define SIMDJSON_ASSUME(COND) __assume(COND)
#else
#define SIMDJSON_UNREACHABLE() __builtin_unreachable();
#define SIMDJSON_ASSUME(COND) do { if (!(COND)) __builtin_unreachable(); } while (0)
#endif

#else // NDEBUG

#define SIMDJSON_UNREACHABLE() assert(0);
#define SIMDJSON_ASSUME(COND) assert(COND)

#endif

#endif // SIMDJSON_PORTABILITY_H
