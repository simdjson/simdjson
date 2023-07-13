#ifndef SIMDJSON_INTERNAL_SIMDPRUNE_TABLES_H
#define SIMDJSON_INTERNAL_SIMDPRUNE_TABLES_H

#include "simdjson/base.h"

#include <cstdint>

namespace simdjson { // table modified and copied from
namespace internal { // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetTable

extern SIMDJSON_DLLIMPORTEXPORT const unsigned char BitsSetTable256mul2[256];

extern SIMDJSON_DLLIMPORTEXPORT const uint8_t pshufb_combine_table[272];

// 256 * 8 bytes = 2kB, easily fits in cache.
extern SIMDJSON_DLLIMPORTEXPORT const uint64_t thintable_epi8[256];

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_SIMDPRUNE_TABLES_H
