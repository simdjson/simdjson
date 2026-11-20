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

// The one or two digits of every value in [0, 100), at data[2 * v]: "d\0"
// below 10, "dd" otherwise. Copying two bytes from a fixed offset lets the
// digit count change only the returned pointer. Reading from four_digits at
// an offset that depends on v < 10 makes GCC branch, which mispredicts on a
// mix of 9- and 10-digit values.
template <class T = void> struct one_or_two_digits_table {
  static const char data[201];
};

template <class T> const char one_or_two_digits_table<T>::data[201] =
    "0\0" "1\0" "2\0" "3\0" "4\0" "5\0" "6\0" "7\0" "8\0" "9\0"
    "10111213141516171819202122232425262728293031323334353637383940414243444546474849"
    "50515253545556575859606162636465666768697071727374757677787980818283848586878889"
    "90919293949596979899";

// v < 100. Writes one or two digits.
simdjson_really_inline char *write_one_or_two_digits(char *b, uint32_t v) noexcept {
  std::memcpy(b, &one_or_two_digits_table<>::data[2 * v], 2);
  return b + 1 + uint32_t(v >= 10);
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
  if (n < 100) {
    return write_one_or_two_digits(b, n);
  }
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
