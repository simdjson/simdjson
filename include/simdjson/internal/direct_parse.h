#ifndef SIMDJSON_INTERNAL_DIRECT_PARSE_H
#define SIMDJSON_INTERNAL_DIRECT_PARSE_H

// Single-pass parsers for JSON atoms and arrays of atoms. They do not build a
// structural index. Except for the double array, they read only buf[0, len)
// (no padding required). Strings are validated as UTF-8 one at a time; a high
// byte outside a string makes them return UNINITIALIZED so the caller can fall
// back to the full parser.
//
// Not part of the public API.

#include "simdjson/base.h"
#include "simdjson/builtin.h"
#include "simdjson/error.h"
#include "simdjson/implementation.h"
#include "simdjson/internal/json_escape.h"
#include "simdjson/internal/numberparsing_tables.h"

#include <cstdint>
#include <cstring>
#include <concepts>
#include <deque>
#include <limits>
#include <list>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>
#ifdef __cpp_lib_inplace_vector
#include <inplace_vector>
#endif

namespace simdjson {
namespace internal {
namespace direct {

simdjson_inline bool is_ws(uint8_t c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Skip ASCII whitespace. Returns false if a non-ASCII byte is seen.
simdjson_inline bool skip_ws(const uint8_t *&p, const uint8_t *end) noexcept {
  while (p < end) {
    if (*p >= 0x80) { return false; }
    if (!is_ws(*p)) { return true; }
    p++;
  }
  return true;
}

// Parse a JSON int64 at p. On success p points at the finisher (or end).
// Returns UNINITIALIZED if this is not an int64 token (float, too big,
// non-ASCII); the caller should not treat that as a hard document error
// unless it has already committed to this token being an integer.
simdjson_inline error_code parse_int64_token(const uint8_t *&p, const uint8_t *end, int64_t &out) noexcept {
  if (p >= end || *p >= 0x80) { return UNINITIALIZED; }
  const bool negative = (*p == '-');
  if (negative) {
    p++;
    if (p >= end) { return UNINITIALIZED; }
  }
  if (*p < '0' || *p > '9') { return UNINITIALIZED; }
  uint64_t i = 0;
  const uint8_t *digits = p;
  if (*p == '0') {
    p++;
  } else {
    while (p < end && *p >= '0' && *p <= '9') {
      if (size_t(p - digits) >= 19) {
        // Too many digits for int64. Consume the rest of the digit run so the
        // caller can see what follows, but report not-an-int64.
        while (p < end && *p >= '0' && *p <= '9') { p++; }
        return UNINITIALIZED;
      }
      i = i * 10 + uint64_t(*p - '0');
      p++;
    }
  }
  const size_t digit_count = size_t(p - digits);
  if (digit_count == 0) { return UNINITIALIZED; }
  if (*digits == '0' && digit_count > 1) { return UNINITIALIZED; }
  if (p < end && *p >= 0x80) { return UNINITIALIZED; }
  if (p < end && (*p == '.' || *p == 'e' || *p == 'E')) { return UNINITIALIZED; }
  if (i > uint64_t(INT64_MAX) + uint64_t(negative)) { return UNINITIALIZED; }
  out = negative ? int64_t(~i + 1) : int64_t(i);
  return SUCCESS;
}

simdjson_inline error_code parse_uint64_token(const uint8_t *&p, const uint8_t *end, uint64_t &out) noexcept {
  // On Demand rejects any sign, including -0.
  if (p >= end || *p < '0' || *p > '9') { return UNINITIALIZED; }
  uint64_t i = 0;
  const uint8_t *digits = p;
  if (*p == '0') {
    p++;
  } else {
    while (p < end && *p >= '0' && *p <= '9') {
      if (size_t(p - digits) >= 20) {
        while (p < end && *p >= '0' && *p <= '9') { p++; }
        return UNINITIALIZED;
      }
      i = i * 10 + uint64_t(*p - '0');
      p++;
    }
  }
  const size_t digit_count = size_t(p - digits);
  if (*digits == '0' && digit_count > 1) { return UNINITIALIZED; }
  if (p < end && *p >= 0x80) { return UNINITIALIZED; }
  if (p < end && (*p == '.' || *p == 'e' || *p == 'E')) { return UNINITIALIZED; }
  // A 20-digit value is in range only if it starts with 1 and did not wrap.
  if (digit_count == 20 && (*digits != '1' || i <= uint64_t(INT64_MAX))) { return UNINITIALIZED; }
  out = i;
  return SUCCESS;
}

simdjson_inline error_code parse_bool_token(const uint8_t *&p, const uint8_t *end, bool &out) noexcept {
  if (p >= end || *p >= 0x80) { return UNINITIALIZED; }
  if (end - p >= 4 && p[0] == 't' && p[1] == 'r' && p[2] == 'u' && p[3] == 'e') {
    p += 4;
    out = true;
    return SUCCESS;
  }
  if (end - p >= 5 && p[0] == 'f' && p[1] == 'a' && p[2] == 'l' && p[3] == 's' && p[4] == 'e') {
    p += 5;
    out = false;
    return SUCCESS;
  }
  return UNINITIALIZED;
}

simdjson_inline error_code parse_null_token(const uint8_t *&p, const uint8_t *end) noexcept {
  if (end - p >= 4 && p[0] == 'n' && p[1] == 'u' && p[2] == 'l' && p[3] == 'l') {
    p += 4;
    return SUCCESS;
  }
  return UNINITIALIZED;
}

// After a value, require only whitespace through end.
simdjson_inline error_code finish_ws_only(const uint8_t *p, const uint8_t *end) noexcept {
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  // Trailing junk: let the full parser report the canonical error.
  if (p != end) { return UNINITIALIZED; }
  return SUCCESS;
}

template <typename T>
simdjson_inline error_code parse_document(const uint8_t *buf, size_t len, T &out);

template <>
simdjson_inline error_code parse_document<int64_t>(const uint8_t *buf, size_t len, int64_t &out) {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  error_code err = parse_int64_token(p, end, out);
  if (err != SUCCESS) { return err; }
  return finish_ws_only(p, end);
}

template <>
simdjson_inline error_code parse_document<uint64_t>(const uint8_t *buf, size_t len, uint64_t &out) {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  error_code err = parse_uint64_token(p, end, out);
  if (err != SUCCESS) { return err; }
  return finish_ws_only(p, end);
}

template <>
simdjson_inline error_code parse_document<bool>(const uint8_t *buf, size_t len, bool &out) {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  error_code err = parse_bool_token(p, end, out);
  if (err != SUCCESS) { return err; }
  return finish_ws_only(p, end);
}

// The first byte at or after p that is '"', '\\', a control character or
// (when stop_on_high is set) non-ASCII, or end.
template <bool stop_on_high>
simdjson_inline const uint8_t *find_string_stop(const uint8_t *p, const uint8_t *end) noexcept {
#if SIMDJSON_EXPERIMENTAL_HAS_SSE2
  // Sixteen bytes at a time with SSE2, which every x64 processor has.
  const __m128i quote_v = _mm_set1_epi8('"');
  const __m128i backslash_v = _mm_set1_epi8('\\');
  const __m128i control_max = _mm_set1_epi8(0x1f);
  while (end - p >= 16) {
    const __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
    const __m128i control = _mm_cmpeq_epi8(_mm_max_epu8(v, control_max), control_max);
    const __m128i stops = _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(v, quote_v), _mm_cmpeq_epi8(v, backslash_v)), control);
    uint32_t bits = uint32_t(_mm_movemask_epi8(stops));
    if (stop_on_high) { bits |= uint32_t(_mm_movemask_epi8(v)); }
    if (bits != 0) { return p + trailing_zeroes(bits); }
    p += 16;
  }
#elif SIMDJSON_EXPERIMENTAL_HAS_NEON
  // Sixteen bytes at a time with NEON. The narrowing shift keeps four bits
  // per byte, so the lowest set bit divided by four is the byte index.
  const uint8x16_t quote_v = vdupq_n_u8('"');
  const uint8x16_t backslash_v = vdupq_n_u8('\\');
  const uint8x16_t space = vdupq_n_u8(0x20);
  const uint8x16_t high = vdupq_n_u8(0x80);
  while (end - p >= 16) {
    const uint8x16_t v = vld1q_u8(p);
    uint8x16_t stops = vorrq_u8(vorrq_u8(vceqq_u8(v, quote_v), vceqq_u8(v, backslash_v)), vcltq_u8(v, space));
    if (stop_on_high) { stops = vorrq_u8(stops, vcgeq_u8(v, high)); }
    const uint64_t bits = vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(stops), 4)), 0);
    if (bits != 0) { return p + (trailing_zeroes(bits) >> 2); }
    p += 16;
  }
#endif
#if !SIMDJSON_IS_BIG_ENDIAN
  // Eight bytes at a time. A byte's high bit is set in hits when it stops the
  // scan. A borrow can only set bits above a real hit, so the lowest set bit
  // is exact.
  constexpr uint64_t ones = 0x0101010101010101ull;
  constexpr uint64_t highs = 0x8080808080808080ull;
  while (end - p >= 8) {
    uint64_t w;
    std::memcpy(&w, p, 8);
    const uint64_t quote = w ^ (ones * '"');
    const uint64_t backslash = w ^ (ones * '\\');
    const uint64_t hits = (((quote - ones) & ~quote) | ((backslash - ones) & ~backslash) |
                           ((w - ones * 0x20) & ~w) | (stop_on_high ? w : 0)) & highs;
    if (hits != 0) { return p + (trailing_zeroes(hits) >> 3); }
    p += 8;
  }
#endif
  while (p < end && *p != '"' && *p != '\\' && *p >= 0x20 && (!stop_on_high || *p < 0x80)) { p++; }
  return p;
}

// Short strings are copied with fixed-size moves: a memcpy of the exact length
// is an out-of-line call whose length branches mispredict.
simdjson_inline void copy_short(char *dst, const uint8_t *src, size_t n) noexcept {
  if (n >= 16) {
    if (n <= 32) {
      std::memcpy(dst, src, 16);
      std::memcpy(dst + n - 16, src + n - 16, 16);
    } else if (n <= 64) {
      std::memcpy(dst, src, 32);
      std::memcpy(dst + n - 32, src + n - 32, 32);
    } else {
      std::memcpy(dst, src, n);
    }
  } else if (n >= 8) {
    std::memcpy(dst, src, 8);
    std::memcpy(dst + n - 8, src + n - 8, 8);
  } else if (n >= 4) {
    std::memcpy(dst, src, 4);
    std::memcpy(dst + n - 4, src + n - 4, 4);
  } else if (n > 0) {
    dst[0] = char(src[0]);
    dst[n >> 1] = char(src[n >> 1]);
    dst[n - 1] = char(src[n - 1]);
  }
}

simdjson_inline void assign_plain_string(std::string &out, const uint8_t *src, size_t n) {
#if SIMDJSON_SUPPORTS_RESIZE_AND_OVERWRITE
  out.resize_and_overwrite(n, [src, n](char *dst, size_t) noexcept {
    copy_short(dst, src, n);
    return n;
  });
#else
  // Not assign(): extra call sites of std::string::assign make GCC stop
  // inlining it in On-Demand string code compiled in the same file. The
  // output is always a fresh string, so there is no buffer to reuse.
  out = std::string(reinterpret_cast<const char *>(src), n);
#endif
}

// String body at s (after the opening quote) that holds an escape or a
// non-ASCII byte. Unescapes like stage 2 with allow_replacement off; any
// escape it would reject, or a control character, returns UNINITIALIZED.
simdjson_never_inline error_code parse_escaped_string_body(const uint8_t *&p, const uint8_t *s, const uint8_t *stop, const uint8_t *end, std::string &out) {
  // Find the closing quote, resuming at stop, the first byte that the caller's
  // ASCII scan rejected. A backslash always covers the next byte. The scan
  // stops on non-ASCII bytes until it meets the first one, so that an ASCII
  // string skips UTF-8 validation.
  const uint8_t *close = stop;
  bool has_escape = false;
  bool has_high = false;
  for (;;) {
    close = has_high ? find_string_stop<false>(close, end) : find_string_stop<true>(close, end);
    if (close == end || *close < 0x20) { return UNINITIALIZED; }
    if (*close == '"') { break; }
    if (*close == '\\') {
      if (end - close < 2) { return UNINITIALIZED; }
      has_escape = true;
      close += 2;
    } else {
      has_high = true;
      close++;
    }
  }
  const size_t n = size_t(close - s);
  if (has_high && !simdjson::validate_utf8(reinterpret_cast<const char *>(s), n)) { return UNINITIALIZED; }
  if (!has_escape) {
    assign_plain_string(out, s, n);
    p = close + 1;
    return SUCCESS;
  }
  // Every escape is at least as long as the bytes it stands for.
  out.resize(n);
  uint8_t *const dst_start = reinterpret_cast<uint8_t *>(&out[0]);
  uint8_t *dst = dst_start;
  const uint8_t *src = s;
  while (src < close) {
    // Between src and close, the only stop is a backslash: quotes are escaped
    // and control characters were rejected by the scan above.
    const uint8_t *backslash = find_string_stop<false>(src, close);
    std::memcpy(dst, src, size_t(backslash - src));
    dst += backslash - src;
    src = backslash;
    if (src == close) { break; }
    uint8_t c = src[1];
    if (c == 'u') {
      namespace jsoncharutils = simdjson::builtin::jsoncharutils;
      if (close - src < 6) { return UNINITIALIZED; }
      uint32_t code_point = jsoncharutils::hex_to_u32_nocheck(src + 2);
      src += 6;
      if (code_point >= 0xd800 && code_point < 0xdc00) {
        if (close - src < 6 || src[0] != '\\' || src[1] != 'u') { return UNINITIALIZED; }
        const uint32_t low_bit = jsoncharutils::hex_to_u32_nocheck(src + 2) - 0xdc00;
        if (low_bit >> 10) { return UNINITIALIZED; }
        code_point = (((code_point - 0xd800) << 10) | low_bit) + 0x10000;
        src += 6;
      } else if (code_point >= 0xdc00 && code_point <= 0xdfff) {
        return UNINITIALIZED;
      }
      const size_t written = jsoncharutils::codepoint_to_utf8(code_point, dst);
      if (written == 0) { return UNINITIALIZED; }
      dst += written;
      continue;
    }
    switch (c) {
      case '"': case '\\': case '/': break;
      case 'b': c = '\b'; break;
      case 'f': c = '\f'; break;
      case 'n': c = '\n'; break;
      case 'r': c = '\r'; break;
      case 't': c = '\t'; break;
      default: return UNINITIALIZED;
    }
    *dst++ = c;
    src += 2;
  }
  out.resize(size_t(dst - dst_start));
  p = close + 1;
  return SUCCESS;
}

// JSON string token. Plain ASCII strings take the inline path; escapes and
// non-ASCII bytes go through parse_escaped_string_body.
simdjson_inline error_code parse_string_token(const uint8_t *&p, const uint8_t *end, std::string &out) {
  if (p >= end || *p != '"') { return UNINITIALIZED; }
  const uint8_t *start = p + 1;
  const uint8_t *stop = find_string_stop<true>(start, end);
  if (simdjson_likely(stop < end && *stop == '"')) {
    assign_plain_string(out, start, size_t(stop - start));
    p = stop + 1;
    return SUCCESS;
  }
  return parse_escaped_string_body(p, start, stop, end, out);
}

// The standard containers that the array parsers below fill. Other types,
// including types derived from these, may have their own deserializer and go
// through On Demand.
template <typename V> struct is_direct_sequence : std::false_type {};
template <typename E, typename A> struct is_direct_sequence<std::vector<E, A>> : std::true_type {};
template <typename E, typename A> struct is_direct_sequence<std::deque<E, A>> : std::true_type {};
template <typename E, typename A> struct is_direct_sequence<std::list<E, A>> : std::true_type {};
#ifdef __cpp_lib_inplace_vector
template <typename E, size_t N> struct is_direct_sequence<std::inplace_vector<E, N>> : std::true_type {};
#endif

template <typename V> struct is_inplace_vector : std::false_type {};
#ifdef __cpp_lib_inplace_vector
template <typename E, size_t N> struct is_inplace_vector<std::inplace_vector<E, N>> : std::true_type {};
#endif

template <typename V> struct is_direct_set : std::false_type {};
template <typename E, typename C, typename A> struct is_direct_set<std::set<E, C, A>> : std::true_type {};
template <typename E, typename C, typename A> struct is_direct_set<std::multiset<E, C, A>> : std::true_type {};
template <typename E, typename H, typename K, typename A> struct is_direct_set<std::unordered_set<E, H, K, A>> : std::true_type {};
template <typename E, typename H, typename K, typename A> struct is_direct_set<std::unordered_multiset<E, H, K, A>> : std::true_type {};

template <typename V, typename E>
concept appendable_container_of =
    (is_direct_sequence<V>::value || is_direct_set<V>::value) && std::same_as<typename V::value_type, E>;

// A std::inplace_vector that is full makes the parsers below fall back to On
// Demand, which reports CAPACITY.
template <typename V>
simdjson_inline bool is_full(const V &out) noexcept {
  if constexpr (is_inplace_vector<V>::value) {
    return out.size() >= out.max_size();
  } else {
    (void)out;
    return false;
  }
}

// Inserts as On Demand does: at the back of a sequence, with emplace into a set.
template <typename V, typename... Args>
simdjson_inline void append_one(V &out, Args &&...args) {
  if constexpr (is_direct_set<V>::value) {
    out.emplace(std::forward<Args>(args)...);
  } else {
    out.emplace_back(std::forward<Args>(args)...);
  }
}

template <typename V> struct is_std_vector : std::false_type {};
template <typename E, typename A> struct is_std_vector<std::vector<E, A>> : std::true_type {};

// Appends the JSON string token at p to out. A plain ASCII string is
// constructed in place.
template <typename V>
simdjson_inline error_code append_string_token(const uint8_t *&p, const uint8_t *end, V &out) {
  if (p >= end || *p != '"') { return UNINITIALIZED; }
  const uint8_t *start = p + 1;
  const uint8_t *stop = find_string_stop<true>(start, end);
  if (simdjson_likely(stop < end && *stop == '"')) {
    append_one(out, reinterpret_cast<const char *>(start), size_t(stop - start));
    p = stop + 1;
    return SUCCESS;
  }
  if constexpr (is_direct_set<V>::value) {
    std::string value;
    error_code err = parse_escaped_string_body(p, start, stop, end, value);
    if (simdjson_likely(err == SUCCESS)) { append_one(out, std::move(value)); }
    return err;
  } else {
    out.emplace_back();
    return parse_escaped_string_body(p, start, stop, end, out.back());
  }
}

// Containers are sized from the first element, which can underestimate the
// width of the others by a lot (["", "<long string>"]). The reservation is
// capped at four times the input size and trimmed when it is mostly unused.
// Only std::vector reserves: std::inplace_vector::reserve throws beyond its
// capacity, and std::deque and std::list have no reserve.
template <typename V>
simdjson_inline void reserve_from_first(V &out, size_t rest, size_t stride) {
  if constexpr (is_std_vector<V>::value) {
    const size_t estimate = rest / stride + 2;
    const size_t limit = 4 * rest / sizeof(typename V::value_type) + 2;
    out.reserve(estimate < limit ? estimate : limit);
  }
}

template <typename V>
simdjson_inline void trim_reservation(V &out) {
  if constexpr (is_std_vector<V>::value) {
    if (out.capacity() / 2 > out.size() + 16) { out.shrink_to_fit(); }
  }
}

template <>
simdjson_inline error_code parse_document<std::string>(const uint8_t *buf, size_t len, std::string &out) {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  error_code err = parse_string_token(p, end, out);
  if (err != SUCCESS) { return err; }
  return finish_ws_only(p, end);
}

// Array of strings. Same fallback rules as parse_string_token.
template <typename V>
  requires appendable_container_of<V, std::string>
simdjson_inline error_code parse_document(const uint8_t *buf, size_t len, V &out) {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  if (*p != '[') { return UNINITIALIZED; }
  p++;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  out.clear();
  if (p < end && *p == ']') {
    p++;
    return finish_ws_only(p, end);
  }
  {
    // Size the buffer from the first element; later strings are built in place.
    const uint8_t *tok = p;
    std::string first;
    if (parse_string_token(p, end, first) != SUCCESS) { return UNINITIALIZED; }
    reserve_from_first(out, size_t(end - tok), size_t(p - tok) + 1); // string plus comma
    if (is_full(out)) { return UNINITIALIZED; }
    append_one(out, std::move(first));
  }
  for (;;) {
    if (!skip_ws(p, end) || p >= end) { out.clear(); return UNINITIALIZED; }
    if (*p == ']') {
      p++;
      if (finish_ws_only(p, end) != SUCCESS) { out.clear(); return UNINITIALIZED; }
      trim_reservation(out);
      return SUCCESS;
    }
    if (*p != ',') { out.clear(); return UNINITIALIZED; }
    p++;
    if (!skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
    if (is_full(out) || append_string_token(p, end, out) != SUCCESS) { out.clear(); return UNINITIALIZED; }
  }
}

// A JSON number at p as a double, with the digit loops and rounding of the On
// Demand number parser. Those read ahead (eight bytes at a time) and stop at
// the first byte that cannot continue a number, so the caller must guarantee
// such a byte before the buffer's padding runs out. Anything that is not a
// finite JSON number returns UNINITIALIZED and leaves p alone.
simdjson_inline error_code parse_double_padded(const uint8_t *&p, double &out) noexcept {
  namespace numberparsing = simdjson::builtin::numberparsing;
  const uint8_t *const start = p;
  const bool negative = (*start == '-');
  const uint8_t *const digits = start + uint8_t(negative);
  const uint8_t *q = digits;
  uint64_t i = 0;
  bool leading_zero;
  numberparsing::parse_float_integer_digits(q, i, leading_zero);
  if (q == digits || (leading_zero && q != digits + 1)) { return UNINITIALIZED; }
  int64_t exponent = 0;
  bool overflow;
  if (*q == '.') {
    q++;
    const uint8_t *const first_fraction = q;
    numberparsing::parse_fraction_digits(q, i);
    if (q == first_fraction) { return UNINITIALIZED; }
    exponent = -(q - first_fraction);
    overflow = q - digits - 1 > 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // 0.000ddd: the zeros after the point do not count.
      const uint8_t *significant = digits + 2;
      while (*significant == '0') { significant++; }
      overflow = q - significant > 19;
    }
  } else {
    overflow = q - digits > 19;
  }
  if (*q == 'e' || *q == 'E') {
    q++;
    const bool exp_negative = (*q == '-');
    q += exp_negative || *q == '+';
    const uint8_t *const first_exp = q;
    uint64_t exp = 0;
    while (numberparsing::parse_digit(*q, exp)) { q++; }
    if (q == first_exp) { return UNINITIALIZED; }
    overflow = overflow || q - first_exp > 19;
    exponent += exp_negative ? 0 - exp : exp;
  }
  overflow = overflow || exponent < simdjson::internal::smallest_power ||
             exponent > simdjson::internal::largest_power;
  double d = 0;
  if (simdjson_unlikely(overflow || !numberparsing::compute_float_64(exponent, i, negative, d))) {
    d = simdjson::internal::from_chars(reinterpret_cast<const char *>(start),
                                       reinterpret_cast<const char *>(q));
    if (!(d >= std::numeric_limits<double>::lowest() && d <= (std::numeric_limits<double>::max)())) {
      return UNINITIALIZED;
    }
  }
  out = d;
  p = q;
  return SUCCESS;
}

// Array of doubles. Needs SIMDJSON_PADDING readable bytes after len: numbers
// are read with parse_double_padded, and requiring the last non-space byte to
// be the closing bracket bounds every number scan.
template <typename V>
  requires appendable_container_of<V, double>
simdjson_inline error_code parse_padded_document(const uint8_t *buf, size_t len, V &out) {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  if (*p != '[') { return UNINITIALIZED; }
  const uint8_t *last = end;
  while (is_ws(last[-1])) { last--; }
  if (last - 1 == p || last[-1] != ']') { return UNINITIALIZED; }
  p++;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  out.clear();
  if (*p == ']') {
    p++;
    return finish_ws_only(p, end);
  }
  bool reserved = false;
  // Every number and separator ends at or before the closing bracket, so
  // *p stays in bounds without checking end.
  for (;;) {
    const uint8_t *tok = p;
    double v;
    if (parse_double_padded(p, v) != SUCCESS) { out.clear(); return UNINITIALIZED; }
    if (!reserved) {
      reserve_from_first(out, size_t(end - tok), size_t(p - tok) + 1);
      reserved = true;
    }
    if (is_full(out)) { out.clear(); return UNINITIALIZED; }
    append_one(out, v);
    if (*p != ',') {
      if (!skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
      if (*p == ']') {
        p++;
        if (finish_ws_only(p, end) != SUCCESS) { out.clear(); return UNINITIALIZED; }
        trim_reservation(out);
        return SUCCESS;
      }
      if (*p != ',') { out.clear(); return UNINITIALIZED; }
    }
    p++;
    if (is_ws(*p) && !skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
  }
}

} // namespace direct
} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_DIRECT_PARSE_H
