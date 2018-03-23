#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

#include <x86intrin.h>
#include <immintrin.h>

typedef __m128i m128;
typedef __m256i m256;

// Snippets from Hyperscan

// Align to N-byte boundary
#define ROUNDUP_N(a, n) (((a) + ((n)-1)) & ~((n)-1))
#define ROUNDDOWN_N(a, n) ((a) & ~((n)-1))

#define ISALIGNED_N(ptr, n) (((uintptr_t)(ptr) & ((n) - 1)) == 0)

#define really_inline inline __attribute__ ((always_inline, unused))
#define never_inline inline __attribute__ ((noinline, unused))

#ifndef likely
  #define likely(x)     __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
  #define unlikely(x)   __builtin_expect(!!(x), 0)
#endif

static inline
u32 ctz64(u64 x) {
	assert(x); // behaviour not defined for x == 0
#if defined(_WIN64)
	unsigned long r;
	_BitScanForward64(&r, x);
	return r;
#elif defined(_WIN32)
	unsigned long r;
	if (_BitScanForward(&r, (u32)x)) {
		return (u32)r;
	}
	_BitScanForward(&r, x >> 32);
	return (u32)(r + 32);
#else
	return (u32)__builtin_ctzll(x);
#endif
}
