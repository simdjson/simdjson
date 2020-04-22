#ifndef SIMDJSON_PORTABILITY_H
#define SIMDJSON_PORTABILITY_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#ifdef _MSC_VER
#include <iso646.h>
#endif
#include <stdio.h>
#include <string.h>
#include <cerrno>
#include <utility>
#include <limits>
#include <stdarg.h>

#if defined(__x86_64__) || defined(_M_AMD64)
#define IS_X86_64 1
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
#define IS_ARM64 1
#endif

// this is almost standard?
#undef STRINGIFY_IMPLEMENTATION_
#undef STRINGIFY
#define STRINGIFY_IMPLEMENTATION_(a) #a
#define STRINGIFY(a) STRINGIFY_IMPLEMENTATION_(a)

#ifndef SIMDJSON_IMPLEMENTATION_FALLBACK
#define SIMDJSON_IMPLEMENTATION_FALLBACK 1
#endif

#if IS_ARM64
#ifndef SIMDJSON_IMPLEMENTATION_ARM64
#define SIMDJSON_IMPLEMENTATION_ARM64 1
#endif
#define SIMDJSON_IMPLEMENTATION_HASWELL 0
#define SIMDJSON_IMPLEMENTATION_WESTMERE 0
#endif // IS_ARM64

#if IS_X86_64
#ifndef SIMDJSON_IMPLEMENTATION_HASWELL
#define SIMDJSON_IMPLEMENTATION_HASWELL 1
#endif
#ifndef SIMDJSON_IMPLEMENTATION_WESTMERE
#define SIMDJSON_IMPLEMENTATION_WESTMERE 1
#endif
#define SIMDJSON_IMPLEMENTATION_ARM64 0
#endif // IS_X86_64

// we are going to use runtime dispatch
#ifdef IS_X86_64
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

// Threading is disabled
#undef SIMDJSON_THREADS_ENABLED
// Is threading enabled?
#if defined(BOOST_HAS_THREADS) || defined(_REENTRANT) || defined(_MT)
#define SIMDJSON_THREADS_ENABLED
#endif


// workaround for large stack sizes under -O0.
// https://github.com/simdjson/simdjson/issues/691
#ifdef __APPLE__
#ifndef __OPTIMIZE__
// Apple systems have small stack sizes in secondary threads.
// Lack of compiler optimization may generate high stack usage.
// So we are disabling multithreaded support for safety.
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

#ifdef _MSC_VER
#include <intrin.h> // visual studio
#endif

#ifdef _MSC_VER
#define simdjson_strcasecmp _stricmp
#else
#define simdjson_strcasecmp strcasecmp
#endif

namespace simdjson {

/** @private portable version of  posix_memalign */
static inline void *aligned_malloc(size_t alignment, size_t size) {
  void *p;
#ifdef _MSC_VER
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
#ifdef _MSC_VER
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

//
// Compatibility functions for Microsoft's *_s functions, so we pass the _CRT_SECURE_WARNING check.
// These are generally not defined on other platforms.
//

namespace internal {

#ifdef _MSC_VER

/** @private */
static inline errno_t fopen_s(FILE **f, const char *name, const char *mode) {
  return ::fopen_s(f, name, mode);
}
/** @private */
static inline errno_t strcpy_s(char *dest, rsize_t destsz, const char *src) {
  return ::strcpy_s(dest, destsz, src);
}
/** @private */
static inline errno_t getenv_s(size_t *len, char *value, rsize_t valuesz, const char *name) {
  return ::getenv_s(len, value, valuesz, name);
}
/** @private */
static inline int sprintf_s(char *buffer, size_t sizeOfBuffer, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsprintf_s(buffer, sizeOfBuffer, format, args);
  va_end(args);
  return result;
}

#else // _MSC_VER

typedef size_t rsize_t;
typedef int errno_t;

/** @private */
static inline errno_t fopen_s(FILE **f, const char *name, const char *mode) {
  errno_t ret = 0;
  assert(f);
  *f = fopen(name, mode);
  if (!*f)
      ret = errno;
  return ret;
}

/** @private */
static inline errno_t strcpy_s(char *dest, rsize_t destsz, const char *src) {
  strncpy(dest, src, destsz);
  if (dest[destsz-1] != '\0') {
    return ERANGE;
  }
  return 0;
}

/** @private */
static inline errno_t getenv_s(size_t *len, char *value, rsize_t valuesz, const char *name) {
  if (valuesz == 0) {
    return EINVAL;
  }

  char *getenv_value = getenv(name);
  if (getenv_value) {
    errno_t error = strcpy_s(value, valuesz, getenv_value);
    if (error) {
      return error;
    }
    if (len) {
      *len = strlen(getenv_value);
    }
  } else {
    memset(value, 0, valuesz);
    if (len) {
      *len = 0;
    }
  }

  return 0;
}

/** @private */
static inline int sprintf_s(char *buffer, size_t sizeOfBuffer, const char *format, ...) {
  if (sizeOfBuffer > std::numeric_limits<int>::max()) {
    errno = EINVAL;
    return -1;
  }
  va_list args;
  va_start(args, format);
  int result = vsnprintf(buffer, sizeOfBuffer, format, args);
  va_end(args);
  if (result >= int(sizeOfBuffer)) {
    errno = EINVAL;
    return -1;
  }
  return result;
}

#endif // _MSC_VER

}

} // namespace simdjson
#endif // SIMDJSON_PORTABILITY_H
