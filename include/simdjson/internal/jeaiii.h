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
simdjson_really_inline void itoa_wr3(char *b, uint32_t v) noexcept {
  const uint32_t hi = v / 100u;
  *b = char('0' + hi);
  itoa_wr2(b + 1, v - hi * 100u);
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

simdjson_really_inline char *write_uint_jeaiii(char *b, uint64_t n) noexcept {
  if (n < 100000000ull) {
    return itoa_lt1e8(b, uint32_t(n));
  }
  const uint64_t hi = itoa_div1e8(n);
  const uint32_t lo = uint32_t(n - hi * 100000000ull);
  if (hi < 100000000ull) {
    b = itoa_lt1e8(b, uint32_t(hi));
    itoa_wr8(b, lo);
    return b + 8;
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
