#ifndef SIMDJSON_INTERNAL_JEAIII_H
#define SIMDJSON_INTERNAL_JEAIII_H

#include "simdjson/base.h"
#include "simdjson/internal/itoa_tables.h"
#include <cstdint>
#include <cstring>

namespace simdjson {
namespace internal {

// Integer to decimal. Two-digit pairs use the classic 00..99 table;
// longer values are split into 4-digit blocks (itoa_t4) with reciprocal
// division by 10^4 and 10^8. The entry point keeps the historical name.
static const char jeaiii_dd[201] =
    "00010203040506070809101112131415161718192021222324252627282930313233343536373839"
    "40414243444546474849505152535455565758596061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

// Length-specialized unsigned decimal writer. Two-digit pairs come from
// jeaiii_dd; four-digit blocks from itoa_t4 (low byte = first digit).
// Divides by 10^4 / 10^8 use reciprocal multiplication.

simdjson_really_inline void itoa_wr2(char *b, uint32_t v) noexcept {
  std::memcpy(b, &jeaiii_dd[2 * v], 2);
}
simdjson_really_inline void itoa_wr4(char *b, uint32_t v) noexcept {
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  const uint32_t t = itoa_t4[v];
  b[0] = char(uint8_t(t));
  b[1] = char(uint8_t(t >> 8));
  b[2] = char(uint8_t(t >> 16));
  b[3] = char(uint8_t(t >> 24));
#else
  // Low byte is the first digit; on little-endian a 4-byte copy matches.
  std::memcpy(b, &itoa_t4[v], 4);
#endif
}
// v < 1000. itoa_t4[v] holds digits 0xyz (first digit in the low 8 bits).
simdjson_really_inline void itoa_wr3(char *b, uint32_t v) noexcept {
  const uint32_t t = itoa_t4[v];
  b[0] = char(uint8_t(t >> 8));
  b[1] = char(uint8_t(t >> 16));
  b[2] = char(uint8_t(t >> 24));
}
simdjson_really_inline void itoa_wr8(char *b, uint32_t v) noexcept {
  // v < 10^8. Always eight digits.
  const uint32_t hi = uint32_t((uint64_t(v) * 3518437209ull) >> 45);
  itoa_wr4(b, hi);
  itoa_wr4(b + 4, v - hi * 10000u);
}
simdjson_really_inline uint64_t itoa_div1e8(uint64_t v) noexcept {
#if defined(__SIZEOF_INT128__)
  return uint64_t((__uint128_t(v) * 12379400392853802749ull) >> 90);
#else
  return v / 100000000ull;
#endif
}

// Writes 1..8 digits. n < 10^8.
simdjson_really_inline char *itoa_lt1e8(char *b, uint32_t n) noexcept {
  if (n < 100u) {
    if (n < 10u) { *b = char('0' + n); return b + 1; }
    itoa_wr2(b, n);
    return b + 2;
  }
  if (n < 10000u) {
    if (n < 1000u) { itoa_wr3(b, n); return b + 3; }
    itoa_wr4(b, n);
    return b + 4;
  }
  const uint32_t hi = uint32_t((uint64_t(n) * 3518437209ull) >> 45);
  const uint32_t lo = n - hi * 10000u;
  if (n < 1000000u) {
    if (n < 100000u) { *b = char('0' + hi); itoa_wr4(b + 1, lo); return b + 5; }
    itoa_wr2(b, hi);
    itoa_wr4(b + 2, lo);
    return b + 6;
  }
  if (n < 10000000u) { itoa_wr3(b, hi); itoa_wr4(b + 3, lo); return b + 7; }
  itoa_wr4(b, hi);
  itoa_wr4(b + 4, lo);
  return b + 8;
}

// 10/11/12-digit kernels. `lo` is the low 8 digits. The 11-digit
// form writes the 3-digit head as a 4-byte store (third digit's
// neighbour is a zero) and the following 4-digit block overwrites
// that zero: one store instead of three byte extracts. Little-endian
// only for that overlap; big-endian uses itoa_wr3.
simdjson_really_inline char *itoa_d10(char *b, uint32_t hi, uint32_t lo) noexcept {
  // hi is 10..99
  itoa_wr2(b, hi);
  itoa_wr8(b + 2, lo);
  return b + 10;
}
simdjson_really_inline char *itoa_d11(char *b, uint32_t hi, uint32_t lo) noexcept {
  // hi is 100..999
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  itoa_wr3(b, hi);
#else
  const uint32_t t = itoa_t4[hi] >> 8; // digits x,y,z and a 0
  std::memcpy(b, &t, 4);
#endif
  itoa_wr8(b + 3, lo);
  return b + 11;
}
simdjson_really_inline char *itoa_d12(char *b, uint32_t hi, uint32_t lo) noexcept {
  // hi is 1000..9999
  itoa_wr4(b, hi);
  itoa_wr8(b + 4, lo);
  return b + 12;
}

simdjson_really_inline char *write_uint_jeaiii(char *b, uint64_t n) noexcept {
  if (n < 100000000ull) {
    return itoa_lt1e8(b, uint32_t(n));
  }
  const uint64_t hi = itoa_div1e8(n);
  const uint32_t lo = uint32_t(n - hi * 100000000ull);
  if (hi < 100000000ull) {
    // 9..16 digits. 10/11/12 are straight-line; the head width is
    // known from n, so we do not re-enter the 1..8-digit tree.
    if (n < 1000000000000ull) {
      if (n < 10000000000ull) {
        if (n < 1000000000ull) {
          // 9 digits, hi is 1..9
          *b = char('0' + uint32_t(hi));
          itoa_wr8(b + 1, lo);
          return b + 9;
        }
        return itoa_d10(b, uint32_t(hi), lo);
      }
      if (n < 100000000000ull) {
        return itoa_d11(b, uint32_t(hi), lo);
      }
      return itoa_d12(b, uint32_t(hi), lo);
    }
    // 13..16 digits. Head is hi (5..8 digits); low 8 digits are lo.
    // Straight-line heads avoid a second trip through itoa_lt1e8.
    const uint32_t h = uint32_t(hi);
    const uint32_t a = uint32_t((uint64_t(h) * 3518437209ull) >> 45); // h / 10000
    const uint32_t hlo = h - a * 10000u;
    if (n < 100000000000000ull) {
      if (n < 10000000000000ull) {
        // 13: head is 5 digits (a is 1..9, hlo is 4 digits)
        *b = char('0' + a);
        itoa_wr4(b + 1, hlo);
        itoa_wr8(b + 5, lo);
        return b + 13;
      }
      // 14: head is 6 digits (a is 10..99)
      itoa_wr2(b, a);
      itoa_wr4(b + 2, hlo);
      itoa_wr8(b + 6, lo);
      return b + 14;
    }
    if (n < 1000000000000000ull) {
      // 15: head is 7 digits (a is 100..999)
      itoa_wr3(b, a);
      itoa_wr4(b + 3, hlo);
      itoa_wr8(b + 7, lo);
      return b + 15;
    }
    // 16: head is 8 digits
    itoa_wr4(b, a);
    itoa_wr4(b + 4, hlo);
    itoa_wr8(b + 8, lo);
    return b + 16;
  }
  // 17..20 digits: hi itself has 9..12 digits.
  const uint64_t top = itoa_div1e8(hi);
  const uint32_t mid = uint32_t(hi - top * 100000000ull);
  b = itoa_lt1e8(b, uint32_t(top)); // top is 1..1844 for uint64
  itoa_wr8(b, mid);
  itoa_wr8(b + 8, lo);
  return b + 16;
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_JEAIII_H
