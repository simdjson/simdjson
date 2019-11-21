#ifndef SIMDJSON_PORTABILITY_H
#define SIMDJSON_PORTABILITY_H

#if defined(__x86_64__) || defined(_M_AMD64)
#define IS_X86_64 1
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
#define IS_ARM64 1
#endif

// this is almost standard?
#define STRINGIFY(a) #a

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

#endif  // x86

// Default target region macros don't do anything.
#ifndef TARGET_REGION
#define TARGET_REGION(T)
#define UNTARGET_REGION
#endif

// under GCC and CLANG, we use these two macros
#define TARGET_HASWELL TARGET_REGION("avx2,bmi,pclmul")
#define TARGET_WESTMERE TARGET_REGION("sse4.2,pclmul")
#define TARGET_ARM64

// Is threading enabled?
#if defined(BOOST_HAS_THREADS) || defined(_REENTRANT) || defined(_MT)
#define SIMDJSON_THREADS_ENABLED 1
#endif

#ifdef _MSC_VER
#include <intrin.h>
#else
#if IS_X86_64
#include <x86intrin.h>
#elif IS_ARM64
#include <arm_neon.h>
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
/* Microsoft C/C++-compatible compiler */
#include <cstdint>
#include <iso646.h>

namespace simdjson {
static inline bool add_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
  return _addcarry_u64(0, value1, value2,
                       reinterpret_cast<unsigned __int64 *>(result));
}

#pragma intrinsic(_umul128)
static inline bool mul_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
  uint64_t high;
  *result = _umul128(value1, value2, &high);
  return high;
}

static inline int trailing_zeroes(uint64_t input_num) {
  return static_cast<int>(_tzcnt_u64(input_num));
}

static inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return _blsr_u64(input_num);
}

static inline int leading_zeroes(uint64_t input_num) {
  return static_cast<int>(_lzcnt_u64(input_num));
}

static inline int hamming(uint64_t input_num) {
#ifdef _WIN64 // highly recommended!!!
  return (int)__popcnt64(input_num);
#else // if we must support 32-bit Windows
  return (int)(__popcnt((uint32_t)input_num) +
               __popcnt((uint32_t)(input_num >> 32)));
#endif
}
} // namespace simdjson
#else
#include <cstdint>
#include <cstdlib>

namespace simdjson {
static inline bool add_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
  return __builtin_uaddll_overflow(value1, value2,
                                   (unsigned long long *)result);
}
static inline bool mul_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
  return __builtin_umulll_overflow(value1, value2,
                                   (unsigned long long *)result);
}

/* result might be undefined when input_num is zero */
static inline NO_SANITIZE_UNDEFINED int trailing_zeroes(uint64_t input_num) {
#ifdef __BMI__ // tzcnt is BMI1
  return _tzcnt_u64(input_num);
#else
  return __builtin_ctzll(input_num);
#endif
}

/* result might be undefined when input_num is zero */
static inline uint64_t clear_lowest_bit(uint64_t input_num) {
#ifdef __BMI__ // blsr is BMI1
  return _blsr_u64(input_num);
#else
  return input_num & (input_num-1);
#endif
}

/* result might be undefined when input_num is zero */
static inline int leading_zeroes(uint64_t input_num) {
#ifdef __BMI2__
  return _lzcnt_u64(input_num);
#else
  return __builtin_clzll(input_num);
#endif
}

/* result might be undefined when input_num is zero */
static inline int hamming(uint64_t input_num) {
#ifdef __POPCOUNT__
  return _popcnt64(input_num);
#else
  return __builtin_popcountll(input_num);
#endif
}
} // namespace simdjson
#endif // _MSC_VER

#ifdef _MSC_VER
#define simdjson_strcasecmp _stricmp
#else
#define simdjson_strcasecmp strcasecmp
#endif

namespace simdjson {
// portable version of  posix_memalign
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

static inline char *aligned_malloc_char(size_t alignment, size_t size) {
  return (char *)aligned_malloc(alignment, size);
}

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

static inline void aligned_free_char(char *mem_block) {
  aligned_free((void *)mem_block);
}
} // namespace simdjson
#endif // SIMDJSON_PORTABILITY_H
