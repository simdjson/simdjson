#ifndef SIMDJSON_PORTABILITY_H
#define SIMDJSON_PORTABILITY_H

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#include <iso646.h>
#include <cstdint>

static inline bool add_overflow(uint64_t value1, uint64_t value2, uint64_t *result) {
	return _addcarry_u64(0, value1, value2, reinterpret_cast<unsigned __int64 *>(result));
}

#  pragma intrinsic(_umul128)
static inline bool mul_overflow(uint64_t value1, uint64_t value2, uint64_t *result) {
	uint64_t high;
	*result = _umul128(value1, value2, &high);
	return high;
}

static inline int trailingzeroes(uint64_t input_num) {
    return _tzcnt_u64(input_num);
}

static inline int leadingzeroes(uint64_t  input_num) {
    return _lzcnt_u64(input_num);
}

static inline int hamming(uint64_t input_num) {
#ifdef _WIN64  // highly recommended!!!
	return (int)__popcnt64(input_num);
#else  // if we must support 32-bit Windows
	return (int)(__popcnt((uint32_t)input_num) +
		__popcnt((uint32_t)(input_num >> 32)));
#endif
}

#else
#include <cstdint>
#include <x86intrin.h>

static inline bool add_overflow(uint64_t  value1, uint64_t  value2, uint64_t *result) {
	return __builtin_uaddll_overflow(value1, value2, (unsigned long long*)result);
}
static inline bool mul_overflow(uint64_t  value1, uint64_t  value2, uint64_t *result) {
	return __builtin_umulll_overflow(value1, value2, (unsigned long long *)result);
}

/* result might be undefined when input_num is zero */
static inline int trailingzeroes(uint64_t input_num) {
#ifdef __BMI__
	return _tzcnt_u64(input_num);
#else
#warning "BMI is missing?"
	return __builtin_ctzll(input_num);
#endif
}

/* result might be undefined when input_num is zero */
static inline int leadingzeroes(uint64_t  input_num) {
	return _lzcnt_u64(input_num);
}

/* result might be undefined when input_num is zero */
static inline int hamming(uint64_t input_num) {
	return _popcnt64(input_num);
}

#endif // _MSC_VER



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
	if (posix_memalign(&p, alignment, size) != 0) { return nullptr; }
#endif
	return p;
}


#ifndef __clang__
#ifndef _MSC_VER
static __m256i inline _mm256_loadu2_m128i(__m128i const *__addr_hi,
                                          __m128i const *__addr_lo) {
  __m256i __v256 = _mm256_castsi128_si256(_mm_loadu_si128(__addr_lo));
  return _mm256_insertf128_si256(__v256, _mm_loadu_si128(__addr_hi), 1);
}

static inline void _mm256_storeu2_m128i(__m128i *__addr_hi, __m128i *__addr_lo,
                                        __m256i __a) {
  __m128i __v128;

  __v128 = _mm256_castsi256_si128(__a);
  _mm_storeu_si128(__addr_lo, __v128);
  __v128 = _mm256_extractf128_si256(__a, 1);
  _mm_storeu_si128(__addr_hi, __v128);
}
#endif
#endif


static inline void aligned_free(void *memblock) {
    if(memblock == nullptr) { return; }
#ifdef _MSC_VER
    _aligned_free(memblock);
#elif defined(__MINGW32__) || defined(__MINGW64__)
    __mingw_aligned_free(memblock);
#else
    free(memblock);
#endif
}

#endif // SIMDJSON_PORTABILITY_H
