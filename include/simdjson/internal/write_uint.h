#ifndef SIMDJSON_INTERNAL_WRITE_UINT_H
#define SIMDJSON_INTERNAL_WRITE_UINT_H

#include "simdjson/base.h"
#include "simdjson/internal/four_digits.h"
#include <cstdint>
#include <cstring>

namespace simdjson {
namespace internal {

// Integer to decimal. Values are cut into groups of four digits, and each
// group is copied from a table. The leading group has one to four digits:
// we count its leading zeros with comparisons and copy four bytes starting
// past them, so that the length of the group costs no branch. Up to three
// bytes past the last digit may be overwritten: callers reserve 20 bytes.

// v < 10^4. Writes exactly four digits.
simdjson_really_inline void write_four_digits(char *b, uint32_t v) noexcept {
  std::memcpy(b, &four_digits<>::data[4 * v], 4);
}

// v < 10^4. Writes one to four digits.
simdjson_really_inline char *write_one_to_four_digits(char *b, uint32_t v) noexcept {
  const uint32_t zeros = uint32_t(v < 10) + uint32_t(v < 100) + uint32_t(v < 1000);
  std::memcpy(b, &four_digits<>::data[4 * v + zeros], 4);
  return b + 4 - zeros;
}

// v < 100. Writes one or two digits.
simdjson_really_inline char *write_one_or_two_digits(char *b, uint32_t v) noexcept {
  const uint32_t zeros = 2 + uint32_t(v < 10);
  std::memcpy(b, &four_digits<>::data[4 * v + zeros], 4);
  return b + 4 - zeros;
}

// v < 10^8. Writes exactly eight digits.
simdjson_really_inline void write_eight_digits(char *b, uint32_t v) noexcept {
  // (v * 109951163) >> 40 is v / 10^4 for v < 10^8.
  const uint32_t high = uint32_t((uint64_t(v) * 109951163) >> 40);
  write_four_digits(b, high);
  write_four_digits(b + 4, v - high * 10000);
}

// n < 10^8. Writes one to eight digits.
simdjson_really_inline char *write_below_1e8(char *b, uint32_t n) noexcept {
  if (n < 10000) {
    return write_one_to_four_digits(b, n);
  }
  const uint32_t high = uint32_t((uint64_t(n) * 109951163) >> 40);
  b = write_one_to_four_digits(b, high);
  write_four_digits(b, n - high * 10000);
  return b + 4;
}

// Writes the decimal digits of n at b and returns a pointer past the last
// digit.
simdjson_really_inline char *write_uint(char *b, uint64_t n) noexcept {
  if (n < 100000000) {
    return write_below_1e8(b, uint32_t(n));
  }
  // At least 9 digits: the low 8 digits, and 1 to 12 digits above them.
  const uint64_t high = n / 100000000;
  const uint32_t low = uint32_t(n - high * 100000000);
  if (high < 100) {
    // 9 or 10 digits, the most common long integers (32-bit values).
    b = write_one_or_two_digits(b, uint32_t(high));
  } else if (high < 100000000) {
    b = write_below_1e8(b, uint32_t(high));
  } else {
    // 17 to 20 digits: the top has 1 to 4 digits.
    const uint64_t top = high / 100000000;
    b = write_one_to_four_digits(b, uint32_t(top));
    write_eight_digits(b, uint32_t(high - top * 100000000));
    b += 8;
  }
  write_eight_digits(b, low);
  return b + 8;
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_WRITE_UINT_H
