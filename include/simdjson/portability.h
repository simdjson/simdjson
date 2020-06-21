#ifndef SIMDJSON_PORTABILITY_H
#define SIMDJSON_PORTABILITY_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>


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
 *  * computed gotos.
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

#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
// https://en.wikipedia.org/wiki/C_alternative_tokens
// This header should have no effect, except maybe
// under Visual Studio.
#include <iso646.h>
#endif

#if defined(__x86_64__) || defined(_M_AMD64)
#define SIMDJSON_IS_X86_64 1
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
#define SIMDJSON_IS_ARM64 1
#endif

#if (!defined(SIMDJSON_IS_X86_64)) && (!defined(SIMDJSON_IS_ARM64))
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
#pragma message("The simdjson library is designed\
 for 64-bit processors and it seems that you are not \
compiling for a known 64-bit platform. All fast kernels \
will be disabled and performance may be poor. Please \
use a 64-bit target such as x64 or 64-bit ARM.")
#else
#error "The simdjson library is designed\
 for 64-bit processors. It seems that you are not \
compiling for a known 64-bit platform."
#endif
#endif // (!defined(SIMDJSON_IS_X86_64)) && (!defined(SIMDJSON_IS_ARM64))

// this is almost standard?
#undef STRINGIFY_IMPLEMENTATION_
#undef STRINGIFY
#define STRINGIFY_IMPLEMENTATION_(a) #a
#define STRINGIFY(a) STRINGIFY_IMPLEMENTATION_(a)

#ifndef SIMDJSON_IMPLEMENTATION_FALLBACK
#define SIMDJSON_IMPLEMENTATION_FALLBACK 1
#endif

#if SIMDJSON_IS_ARM64
#ifndef SIMDJSON_IMPLEMENTATION_ARM64
#define SIMDJSON_IMPLEMENTATION_ARM64 1
#endif
#define SIMDJSON_IMPLEMENTATION_HASWELL 0
#define SIMDJSON_IMPLEMENTATION_WESTMERE 0
#endif // SIMDJSON_IS_ARM64

#if SIMDJSON_IS_X86_64
#ifndef SIMDJSON_IMPLEMENTATION_HASWELL
#define SIMDJSON_IMPLEMENTATION_HASWELL 1
#endif
#ifndef SIMDJSON_IMPLEMENTATION_WESTMERE
#define SIMDJSON_IMPLEMENTATION_WESTMERE 1
#endif
#define SIMDJSON_IMPLEMENTATION_ARM64 0
#endif // SIMDJSON_IS_X86_64

// we are going to use runtime dispatch
#ifdef SIMDJSON_IS_X86_64
#ifdef __clang__
// clang does not have GCC push pop
// warning: clang attribute push can't be used within a namespace in clang up
// til 8.0 so TARGET_REGION and UNTARGET_REGION must be *outside* of a
// namespace.
#define TARGET_REGION(T)                                                       \
  _Pragma(STRINGIFY(                                                           \
      clang attribute push(__attribute__((target(T))), apply_to = function)))
#define UNTARGET_REGION _Pragma("clang attribute pop")
#elif defined(__GNUC__)
// GCC is easier
#define TARGET_REGION(T)                                                       \
  _Pragma("GCC push_options") _Pragma(STRINGIFY(GCC target(T)))
#define UNTARGET_REGION _Pragma("GCC pop_options")
#endif // clang then gcc

#endif // x86

// Default target region macros don't do anything.
#ifndef TARGET_REGION
#define TARGET_REGION(T)
#define UNTARGET_REGION
#endif

// under GCC and CLANG, we use these two macros
#define TARGET_HASWELL TARGET_REGION("avx2,bmi,pclmul,lzcnt")
#define TARGET_WESTMERE TARGET_REGION("sse4.2,pclmul")
#define TARGET_ARM64

// Is threading enabled?
#if defined(BOOST_HAS_THREADS) || defined(_REENTRANT) || defined(_MT)
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
#define NO_SANITIZE_UNDEFINED __attribute__((no_sanitize("undefined")))
#elif defined(__GNUC__)
#define NO_SANITIZE_UNDEFINED __attribute__((no_sanitize_undefined))
#else
#define NO_SANITIZE_UNDEFINED
#endif

#ifdef SIMDJSON_VISUAL_STUDIO
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

namespace simdjson {
/** @private portable version of  posix_memalign */
static inline void *aligned_malloc(size_t alignment, size_t size) {
  void *p;
#ifdef SIMDJSON_VISUAL_STUDIO
  p = _aligned_malloc(size, alignment);
#elif defined(__MINGW32__) || defined(__MINGW64__)
  p = __mingw_aligned_malloc(size, alignment);
#else
  // somehow, if this is used before including "x86intrin.h", it creates an
  // implicit defined warning.
  if (posix_memalign(&p, alignment, size) != 0) {
    return nullptr;
  }
#endif
  return p;
}

/** @private */
static inline char *aligned_malloc_char(size_t alignment, size_t size) {
  return (char *)aligned_malloc(alignment, size);
}

/** @private */
static inline void aligned_free(void *mem_block) {
  if (mem_block == nullptr) {
    return;
  }
#ifdef SIMDJSON_VISUAL_STUDIO
  _aligned_free(mem_block);
#elif defined(__MINGW32__) || defined(__MINGW64__)
  __mingw_aligned_free(mem_block);
#else
  free(mem_block);
#endif
}

/** @private */
static inline void aligned_free_char(char *mem_block) {
  aligned_free((void *)mem_block);
}

#ifdef NDEBUG

#ifdef SIMDJSON_VISUAL_STUDIO
#define SIMDJSON_UNREACHABLE() __assume(0)
#define SIMDJSON_ASSUME(COND) __assume(COND)
#else
#define SIMDJSON_UNREACHABLE() __builtin_unreachable();
#define SIMDJSON_ASSUME(COND) do { if (!(COND)) __builtin_unreachable(); } while (0)
#endif

#else // NDEBUG

#include <cassert>
#define SIMDJSON_UNREACHABLE() assert(0);
#define SIMDJSON_ASSUME(COND) assert(COND)

#endif

} // namespace simdjson
#endif // SIMDJSON_PORTABILITY_H
