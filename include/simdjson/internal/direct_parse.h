#ifndef SIMDJSON_INTERNAL_DIRECT_PARSE_H
#define SIMDJSON_INTERNAL_DIRECT_PARSE_H

// Single-pass parsers for JSON atoms and arrays of atoms. They do not build a
// structural index. They read only buf[0, len) (no padding required) and
// accept only ASCII (a high byte makes them return UNINITIALIZED so the
// caller can fall back to the full indexer, which owns UTF-8 validation).
//
// Not part of the public API.

#include "simdjson/base.h"
#include "simdjson/error.h"
#include "simdjson/internal/json_escape.h"

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

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
  int64_t tmp;
  const uint8_t *save = p;
  error_code err = parse_int64_token(p, end, tmp);
  if (err != SUCCESS) { return err; }
  if (tmp < 0) { p = save; return UNINITIALIZED; }
  out = uint64_t(tmp);
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
simdjson_inline error_code parse_document(const uint8_t *buf, size_t len, T &out) noexcept;

template <>
simdjson_inline error_code parse_document<int64_t>(const uint8_t *buf, size_t len, int64_t &out) noexcept {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  error_code err = parse_int64_token(p, end, out);
  if (err != SUCCESS) { return err; }
  return finish_ws_only(p, end);
}

template <>
simdjson_inline error_code parse_document<uint64_t>(const uint8_t *buf, size_t len, uint64_t &out) noexcept {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  error_code err = parse_uint64_token(p, end, out);
  if (err != SUCCESS) { return err; }
  return finish_ws_only(p, end);
}

template <>
simdjson_inline error_code parse_document<bool>(const uint8_t *buf, size_t len, bool &out) noexcept {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  error_code err = parse_bool_token(p, end, out);
  if (err != SUCCESS) { return err; }
  return finish_ws_only(p, end);
}

// The first byte at or after p that is '"', '\\', a control character or
// non-ASCII (which needs the UTF-8 validator), or end.
simdjson_inline const uint8_t *find_string_stop(const uint8_t *p, const uint8_t *end) noexcept {
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
                           ((w - ones * 0x20) & ~w) | w) & highs;
    if (hits != 0) { return p + (trailing_zeroes(hits) >> 3); }
    p += 8;
  }
#endif
  while (p < end && *p != '"' && *p != '\\' && *p >= 0x20 && *p < 0x80) { p++; }
  return p;
}

constexpr size_t not_plain = size_t(-1);

// Length of the string body at s up to its closing quote, or not_plain if an
// escape, a control character or a non-ASCII byte comes first, or the input
// ends.
simdjson_inline size_t scan_plain_string(const uint8_t *s, const uint8_t *end) noexcept {
  const uint8_t *stop = find_string_stop(s, end);
  return (stop < end && *stop == '"') ? size_t(stop - s) : not_plain;
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

simdjson_inline void assign_plain_string(std::string &out, const uint8_t *src, size_t n) noexcept {
#if SIMDJSON_SUPPORTS_RESIZE_AND_OVERWRITE
  out.resize_and_overwrite(n, [src, n](char *dst, size_t) noexcept {
    copy_short(dst, src, n);
    return n;
  });
#else
  out.assign(reinterpret_cast<const char *>(src), n);
#endif
}

// JSON string with no escapes, no controls, pure ASCII. Anything else falls
// back to the full parser.
simdjson_inline error_code parse_plain_string_token(const uint8_t *&p, const uint8_t *end, std::string &out) noexcept {
  if (p >= end || *p != '"') { return UNINITIALIZED; }
  const uint8_t *start = p + 1;
  const size_t n = scan_plain_string(start, end);
  if (n == not_plain) { return UNINITIALIZED; }
  assign_plain_string(out, start, n);
  p = start + n + 1;
  return SUCCESS;
}

template <>
simdjson_inline error_code parse_document<std::string>(const uint8_t *buf, size_t len, std::string &out) noexcept {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  error_code err = parse_plain_string_token(p, end, out);
  if (err != SUCCESS) { return err; }
  return finish_ws_only(p, end);
}

// Array of int64: [ ws? int (ws? , ws? int)* ws? ]
// On UNINITIALIZED the output vector is cleared and the caller must fall back.
template <>
simdjson_inline error_code parse_document<std::vector<int64_t>>(const uint8_t *buf, size_t len, std::vector<int64_t> &out) noexcept {
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
  bool reserved = false;
  for (;;) {
    int64_t v;
    const uint8_t *tok = p;
    error_code err = parse_int64_token(p, end, v);
    // Any token we cannot accept as int64, including a truncated or
    // malformed number, falls back so the error code stays the full
    // parser's.
    if (err != SUCCESS) { out.clear(); return UNINITIALIZED; }
    if (!reserved) {
      // Size the buffer from the first element so a non-int array does
      // not allocate, and a fresh vector does not geometrically realloc.
      // +2 covers the comma/bracket around each subsequent element.
      const size_t elem = size_t(p - tok) + 2;
      const size_t rest = size_t(end - tok);
      out.reserve(elem ? rest / elem + 2 : 8);
      reserved = true;
    }
    out.push_back(v);
    if (!skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
    if (p >= end) { out.clear(); return UNINITIALIZED; }
    if (*p == ',') {
      p++;
      if (!skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
      continue;
    }
    if (*p == ']') {
      p++;
      error_code fin = finish_ws_only(p, end);
      if (fin != SUCCESS) { out.clear(); return UNINITIALIZED; }
      return SUCCESS;
    }
    out.clear();
    return UNINITIALIZED;
  }
}

// Array of plain ASCII strings (no escapes, no controls). Same fallback
// rules as parse_plain_string_token.
template <>
simdjson_inline error_code parse_document<std::vector<std::string>>(const uint8_t *buf, size_t len, std::vector<std::string> &out) noexcept {
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
    if (parse_plain_string_token(p, end, first) != SUCCESS) { return UNINITIALIZED; }
    const size_t elem = size_t(p - tok) + 1; // string plus comma
    out.reserve(size_t(end - tok) / elem + 2);
    out.push_back(std::move(first));
  }
  for (;;) {
    if (!skip_ws(p, end) || p >= end) { out.clear(); return UNINITIALIZED; }
    if (*p == ']') {
      p++;
      if (finish_ws_only(p, end) != SUCCESS) { out.clear(); return UNINITIALIZED; }
      return SUCCESS;
    }
    if (*p != ',') { out.clear(); return UNINITIALIZED; }
    p++;
    if (!skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
    out.emplace_back();
    if (parse_plain_string_token(p, end, out.back()) != SUCCESS) { out.clear(); return UNINITIALIZED; }
  }
}

// Flat object { "key": <int64>, ... } with plain ASCII keys. First key wins
// on duplicates (same as emplace into a map). Nested values, escapes, or
// non-ASCII fall back.
template <typename Map>
simdjson_inline error_code parse_flat_int64_object(const uint8_t *buf, size_t len, Map &out) noexcept {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  if (*p != '{') { return UNINITIALIZED; }
  p++;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  out.clear();
  if (p < end && *p == '}') {
    p++;
    return finish_ws_only(p, end) == SUCCESS ? SUCCESS : UNINITIALIZED;
  }
  for (;;) {
    std::string key;
    if (parse_plain_string_token(p, end, key) != SUCCESS) { out.clear(); return UNINITIALIZED; }
    if (!skip_ws(p, end) || p >= end || *p != ':') { out.clear(); return UNINITIALIZED; }
    p++;
    if (!skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
    int64_t v;
    if (parse_int64_token(p, end, v) != SUCCESS) { out.clear(); return UNINITIALIZED; }
    out.emplace(std::move(key), v);
    if (!skip_ws(p, end) || p >= end) { out.clear(); return UNINITIALIZED; }
    if (*p == ',') {
      p++;
      if (!skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
      continue;
    }
    if (*p == '}') {
      p++;
      if (finish_ws_only(p, end) != SUCCESS) { out.clear(); return UNINITIALIZED; }
      return SUCCESS;
    }
    out.clear();
    return UNINITIALIZED;
  }
}

template <>
simdjson_inline error_code parse_document<std::map<std::string, int64_t>>(const uint8_t *buf, size_t len, std::map<std::string, int64_t> &out) noexcept {
  return parse_flat_int64_object(buf, len, out);
}
template <>
simdjson_inline error_code parse_document<std::unordered_map<std::string, int64_t>>(const uint8_t *buf, size_t len, std::unordered_map<std::string, int64_t> &out) noexcept {
  return parse_flat_int64_object(buf, len, out);
}

} // namespace direct
} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_DIRECT_PARSE_H
