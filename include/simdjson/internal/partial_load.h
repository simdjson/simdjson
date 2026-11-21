#ifndef SIMDJSON_INTERNAL_PARTIAL_LOAD_H
#define SIMDJSON_INTERNAL_PARTIAL_LOAD_H

#include "simdjson/base.h"

#include <cstdint>
#include <cstring>

namespace simdjson {
namespace internal {

// shift_tail_mask + 16 - n holds the byte indexes that move the last n bytes
// of a 16-byte vector to its front, followed by indexes with the high bit set.
alignas(16) static const uint8_t shift_tail_mask[32] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};

// Loads the n < 16 bytes at ptr, and only those, into two little-endian
// words. The other bytes are set to fill.
simdjson_inline void load_small_words(const uint8_t *ptr, size_t n, uint8_t fill, uint64_t &lo, uint64_t &hi) {
  const uint64_t fills = 0x0101010101010101 * fill;
  lo = fills;
  hi = fills;
  if (n >= 8) {
    std::memcpy(&lo, ptr, 8);
    if (n > 8) {
      // The 8 bytes ending at ptr + n overlap lo: keep their last n - 8.
      uint64_t tail;
      std::memcpy(&tail, ptr + n - 8, 8);
      hi = (tail >> (8 * (16 - n))) | (fills << (8 * (n - 8)));
    }
  } else if (n >= 4) {
    uint32_t first, last;
    std::memcpy(&first, ptr, 4);
    std::memcpy(&last, ptr + n - 4, 4);
    lo = first | (uint64_t(last) << (8 * (n - 4))) | (fills << (8 * n));
  } else if (n > 0) {
    lo = ptr[0] | (uint64_t(ptr[n / 2]) << (8 * (n / 2))) | (uint64_t(ptr[n - 1]) << (8 * (n - 1)))
       | (fills << (8 * n));
  }
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_PARTIAL_LOAD_H
