#ifndef SIMDJSON_COMMON_DEFS_H
#define SIMDJSON_COMMON_DEFS_H

#include <cassert>
#include "simdjson/portability.h"

namespace simdjson {

#ifndef SIMDJSON_EXCEPTIONS
#if __cpp_exceptions
#define SIMDJSON_EXCEPTIONS 1
#else
#define SIMDJSON_EXCEPTIONS 0
#endif
#endif

/** The maximum document size supported by simdjson. */
constexpr size_t SIMDJSON_MAXSIZE_BYTES = 0xFFFFFFFF;

/**
 * The amount of padding needed in a buffer to parse JSON.
 *
 * the input buf should be readable up to buf + SIMDJSON_PADDING
 * this is a stopgap; there should be a better description of the
 * main loop and its behavior that abstracts over this
 * See https://github.com/lemire/simdjson/issues/174
 */
constexpr size_t SIMDJSON_PADDING = 32;

/**
 * By default, simdjson supports this many nested objects and arrays.
 *
 * This is the default for parser::max_depth().
 */
constexpr size_t DEFAULT_MAX_DEPTH = 1024;

} // namespace simdjson

#if defined(__GNUC__)
// Marks a block with a name so that MCA analysis can see it.
#define BEGIN_DEBUG_BLOCK(name) __asm volatile("# LLVM-MCA-BEGIN " #name);
#define END_DEBUG_BLOCK(name) __asm volatile("# LLVM-MCA-END " #name);
#define DEBUG_BLOCK(name, block) BEGIN_DEBUG_BLOCK(name); block; END_DEBUG_BLOCK(name);
#else
#define BEGIN_DEBUG_BLOCK(name)
#define END_DEBUG_BLOCK(name)
#define DEBUG_BLOCK(name, block)
#endif

#if !defined(_MSC_VER) && !defined(SIMDJSON_NO_COMPUTED_GOTO)
// Implemented using Labels as Values which works in GCC and CLANG (and maybe
// also in Intel's compiler), but won't work in MSVC.
#define SIMDJSON_USE_COMPUTED_GOTO
#endif

// Align to N-byte boundary
#define ROUNDUP_N(a, n) (((a) + ((n)-1)) & ~((n)-1))
#define ROUNDDOWN_N(a, n) ((a) & ~((n)-1))

#define ISALIGNED_N(ptr, n) (((uintptr_t)(ptr) & ((n)-1)) == 0)

#ifdef _MSC_VER
#define really_inline __forceinline
#define never_inline __declspec(noinline)

#define UNUSED
#define WARN_UNUSED

#ifndef likely
#define likely(x) x
#endif
#ifndef unlikely
#define unlikely(x) x
#endif

#define SIMDJSON_PUSH_DISABLE_WARNINGS __pragma(warning( push ))
#define SIMDJSON_PUSH_DISABLE_ALL_WARNINGS __pragma(warning( push, 0 ))
#define SIMDJSON_DISABLE_VS_WARNING(WARNING_NUMBER) __pragma(warning( disable : WARNING_NUMBER ))
#define SIMDJSON_DISABLE_DEPRECATED_WARNING SIMDJSON_DISABLE_VS_WARNING(4996)
#define SIMDJSON_POP_DISABLE_WARNINGS __pragma(warning( pop ))

#else // MSC_VER


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

#define SIMDJSON_PUSH_DISABLE_WARNINGS _Pragma("GCC diagnostic push")
// gcc doesn't seem to disable all warnings with all and extra, add warnings here as necessary
#define SIMDJSON_PUSH_DISABLE_ALL_WARNINGS SIMDJSON_PUSH_DISABLE_WARNINGS \
  SIMDJSON_DISABLE_GCC_WARNING(-Wall) \
  SIMDJSON_DISABLE_GCC_WARNING(-Wextra) \
  SIMDJSON_DISABLE_GCC_WARNING(-Wshadow) \
  SIMDJSON_DISABLE_GCC_WARNING(-Wunused-parameter) \
  SIMDJSON_DISABLE_GCC_WARNING(-Wimplicit-fallthrough)
#define SIMDJSON_PRAGMA(P) _Pragma(#P)
#define SIMDJSON_DISABLE_GCC_WARNING(WARNING) SIMDJSON_PRAGMA(GCC diagnostic ignored #WARNING)
#define SIMDJSON_DISABLE_DEPRECATED_WARNING SIMDJSON_DISABLE_GCC_WARNING(-Wdeprecated-declarations)
#define SIMDJSON_POP_DISABLE_WARNINGS _Pragma("GCC diagnostic pop")

#endif // MSC_VER

#endif // SIMDJSON_COMMON_DEFS_H
