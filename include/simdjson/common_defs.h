#ifndef SIMDJSON_COMMON_DEFS_H
#define SIMDJSON_COMMON_DEFS_H

#include "simdjson/portability.h"

#include <cassert>

// we support documents up to 4GB
#define SIMDJSON_MAXSIZE_BYTES 0xFFFFFFFF

// the input buf should be readable up to buf + SIMDJSON_PADDING
#define SIMDJSON_PADDING  sizeof(__m256i)

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

#endif // SIMDJSON_COMMON_DEFS_H
