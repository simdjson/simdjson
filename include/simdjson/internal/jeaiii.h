#ifndef SIMDJSON_INTERNAL_JEAIII_H
#define SIMDJSON_INTERNAL_JEAIII_H

#include "simdjson/base.h"
#include <cstdint>
#include <cstring>

namespace simdjson {
namespace internal {

// Integer to decimal: James Edward Anhalt III's algorithm
static const char jeaiii_dd[201] =
    "00010203040506070809101112131415161718192021222324252627282930313233343536373839"
    "40414243444546474849505152535455565758596061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";
static const char jeaiii_fd[201] =
    "0\0" "1\0" "2\0" "3\0" "4\0" "5\0" "6\0" "7\0" "8\0" "9\0"
    "10111213141516171819202122232425262728293031323334353637383940414243444546474849"
    "50515253545556575859606162636465666768697071727374757677787980818283848586878889"
    "90919293949596979899";

simdjson_really_inline void jeaiii_write_dd(char *p, uint64_t k) noexcept {
  std::memcpy(p, &jeaiii_dd[2 * k], 2);
}
simdjson_really_inline void jeaiii_write_fd(char *p, uint64_t k) noexcept {
  std::memcpy(p, &jeaiii_fd[2 * k], 2);
}

// Caller guarantees n < 10^8. Writes 1 to 8 digits.
simdjson_really_inline char *jeaiii_lt1e8(char *b, uint32_t n) noexcept {
  constexpr uint64_t mask24 = (uint64_t(1) << 24) - 1;
  constexpr uint64_t mask32 = (uint64_t(1) << 32) - 1;
  if (n < 100) {
    jeaiii_write_fd(b, n);
    return n < 10 ? b + 1 : b + 2;
  }
  if (n < 1000000) {
    if (n < 10000) {
      const uint32_t f0 = uint32_t(10 * (1 << 24) / 1e3 + 1) * n;
      jeaiii_write_fd(b, f0 >> 24);
      b -= n < 1000;
      const uint32_t f2 = uint32_t(f0 & mask24) * 100;
      jeaiii_write_dd(b + 2, f2 >> 24);
      return b + 4;
    }
    const uint64_t f0 = uint64_t(10 * (1ull << 32) / 1e5 + 1) * n;
    jeaiii_write_fd(b, f0 >> 32);
    b -= n < 100000;
    const uint64_t f2 = (f0 & mask32) * 100;
    jeaiii_write_dd(b + 2, f2 >> 32);
    const uint64_t f4 = (f2 & mask32) * 100;
    jeaiii_write_dd(b + 4, f4 >> 32);
    return b + 6;
  }
  const uint64_t f0 = uint64_t(10 * (1ull << 48) / 1e7 + 1) * n >> 16;
  jeaiii_write_fd(b, f0 >> 32);
  b -= n < 10000000;
  const uint64_t f2 = (f0 & mask32) * 100;
  jeaiii_write_dd(b + 2, f2 >> 32);
  const uint64_t f4 = (f2 & mask32) * 100;
  jeaiii_write_dd(b + 4, f4 >> 32);
  const uint64_t f6 = (f4 & mask32) * 100;
  jeaiii_write_dd(b + 6, f6 >> 32);
  return b + 8;
}

// Caller guarantees z < 10^8. Always writes exactly 8 digits.
simdjson_really_inline char *jeaiii_8_digits(char *b, uint32_t z) noexcept {
  constexpr uint64_t mask32 = (uint64_t(1) << 32) - 1;
  const uint64_t f0 = (uint64_t((1ull << 48) / 1e6 + 1) * z >> 16) + 1;
  jeaiii_write_dd(b, f0 >> 32);
  const uint64_t f2 = (f0 & mask32) * 100;
  jeaiii_write_dd(b + 2, f2 >> 32);
  const uint64_t f4 = (f2 & mask32) * 100;
  jeaiii_write_dd(b + 4, f4 >> 32);
  const uint64_t f6 = (f4 & mask32) * 100;
  jeaiii_write_dd(b + 6, f6 >> 32);
  return b + 8;
}

// Caller guarantees 10^8 <= n < 2^32. Writes 9 or 10 digits.
simdjson_really_inline char *jeaiii_9_or_10(char *b, uint64_t n) noexcept {
  constexpr uint64_t mask57 = (uint64_t(1) << 57) - 1;
  const uint64_t f0 = uint64_t(10 * (1ull << 57) / 1e9 + 1) * n;
  jeaiii_write_fd(b, f0 >> 57);
  b -= n < 1000000000;
  const uint64_t f2 = (f0 & mask57) * 100;
  jeaiii_write_dd(b + 2, f2 >> 57);
  const uint64_t f4 = (f2 & mask57) * 100;
  jeaiii_write_dd(b + 4, f4 >> 57);
  const uint64_t f6 = (f4 & mask57) * 100;
  jeaiii_write_dd(b + 6, f6 >> 57);
  const uint64_t f8 = (f6 & mask57) * 100;
  jeaiii_write_dd(b + 8, f8 >> 57);
  return b + 10;
}

simdjson_really_inline char *write_uint_jeaiii(char *b, uint64_t n) noexcept {
  if (n < 100000000) {
    return jeaiii_lt1e8(b, uint32_t(n));
  }
  if (n < (uint64_t(1) << 32)) {
    return jeaiii_9_or_10(b, n);
  }
  // At least 10 digits: the low 8 digits, and 2 to 12 digits above them.
  const uint32_t z = uint32_t(n % 100000000);
  uint64_t u = n / 100000000;
  if (u < 100000000) {
    // u has 2 to 8 digits (if u < 10, n would be below 2^32).
    b = jeaiii_lt1e8(b, uint32_t(u));
  } else if (u < (uint64_t(1) << 32)) {
    b = jeaiii_9_or_10(b, u);
  } else {
    // u has 11 or 12 digits: split off 8 more.
    const uint32_t y = uint32_t(u % 100000000);
    u /= 100000000;
    b = jeaiii_lt1e8(b, uint32_t(u)); // 3 or 4 digits
    b = jeaiii_8_digits(b, y);
  }
  return jeaiii_8_digits(b, z);
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_JEAIII_H
