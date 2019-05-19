#ifndef SIMDJSON_COMMON_DEFS_H
#define SIMDJSON_COMMON_DEFS_H

#include "simdjson/portability.h"

#include <cassert>

// we support documents up to 4GB
#define SIMDJSON_MAXSIZE_BYTES 0xFFFFFFFF

// the input buf should be readable up to buf + SIMDJSON_PADDING
#ifdef __AVX2__
#define SIMDJSON_PADDING  sizeof(__m256i)
#else
// this is a stopgap; there should be a better description of the
// main loop and its behavior that abstracts over this
#define SIMDJSON_PADDING  32
#endif

#ifndef _MSC_VER
// Implemented using Labels as Values which works in GCC and CLANG (and maybe
// also in Intel's compiler), but won't work in MSVC.
#define SIMDJSON_USE_COMPUTED_GOTO
#endif


// Align to N-byte boundary
#define ROUNDUP_N(a, n) (((a) + ((n)-1)) & ~((n)-1))
#define ROUNDDOWN_N(a, n) ((a) & ~((n)-1))

#define ISALIGNED_N(ptr, n) (((uintptr_t)(ptr) & ((n)-1)) == 0)

#ifdef _MSC_VER
#define really_inline inline
#define never_inline __declspec(noinline)

#define UNUSED
#define WARN_UNUSED

#ifndef likely
#define likely(x) x
#endif
#ifndef unlikely
#define unlikely(x) x
#endif

#else

// For non-Visual Studio compilers, we may assume that same-page buffer overrun is fine.
// However, it will make it difficult to be "valgrind clean".
//#ifndef ALLOW_SAME_PAGE_BUFFER_OVERRUN
//#define ALLOW_SAME_PAGE_BUFFER_OVERRUN
//#endif 

// The following is likely unnecessarily complex.
#ifdef __SANITIZE_ADDRESS__
// we have GCC, stuck with https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67368
#undef ALLOW_SAME_PAGE_BUFFER_OVERRUN
#elif defined(__has_feature)
// we have CLANG?
#  if (__has_feature(address_sanitizer))
#define ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER  __attribute__((no_sanitize("address")))
#  endif 
#endif 

#if defined(__has_feature)
#  if (__has_feature(memory_sanitizer))
#define LENIENT_MEM_SANITIZER __attribute__((no_sanitize("memory")))
#  endif
#endif

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

#endif  // MSC_VER

// if it does not apply, make it an empty macro
#ifndef ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER
#define ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER
#endif
#ifndef LENIENT_MEM_SANITIZER
#define LENIENT_MEM_SANITIZER
#endif

#endif // SIMDJSON_COMMON_DEFS_H
