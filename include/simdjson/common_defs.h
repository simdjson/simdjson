#pragma once

#include <cassert>

// the input buf should be readable up to buf + SIMDJSON_PADDING
#define SIMDJSON_PADDING  sizeof(__m256i) 




#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <x86intrin.h>
#endif


// Align to N-byte boundary
#define ROUNDUP_N(a, n) (((a) + ((n)-1)) & ~((n)-1))
#define ROUNDDOWN_N(a, n) ((a) & ~((n)-1))

#define ISALIGNED_N(ptr, n) (((uintptr_t)(ptr) & ((n)-1)) == 0)

#ifdef _MSC_VER


#define really_inline inline 
#define never_inline inline 

#define UNUSED 
#define WARN_UNUSED 

#ifndef likely
#define likely(x) 
#endif
#ifndef unlikely
#define unlikely(x) 
#endif

#else

#define really_inline inline __attribute__((always_inline, unused))
#define never_inline inline __attribute__((noinline, unused))

#define UNUSED __attribute__((unused))
#define WARN_UNUSED __attribute__((warn_unused_result))

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#endif