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

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <tuple>
#include <utility>
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

// JSON string with no escapes, no controls, pure ASCII. Anything else
// (including non-ASCII, which needs the UTF-8 validator) falls back.
simdjson_inline error_code parse_plain_string_token(const uint8_t *&p, const uint8_t *end, std::string &out) noexcept {
  if (p >= end || *p != '"') { return UNINITIALIZED; }
  p++;
  const uint8_t *start = p;
  while (p < end) {
    uint8_t c = *p;
    if (c == '"') {
      out.assign(reinterpret_cast<const char *>(start), size_t(p - start));
      p++;
      return SUCCESS;
    }
    // Escape, control, or non-ASCII: full parser.
    if (c == '\\' || c < 0x20 || c >= 0x80) { return UNINITIALIZED; }
    p++;
  }
  return UNINITIALIZED;
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
  bool reserved = false;
  for (;;) {
    const uint8_t *tok = p;
    std::string v;
    if (parse_plain_string_token(p, end, v) != SUCCESS) { out.clear(); return UNINITIALIZED; }
    if (!reserved) {
      const size_t elem = size_t(p - tok) + 1; // string plus comma
      const size_t rest = size_t(end - tok);
      out.reserve(elem ? rest / elem + 2 : 8);
      reserved = true;
    }
    out.push_back(std::move(v));
    if (!skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
    if (p >= end) { out.clear(); return UNINITIALIZED; }
    if (*p == ',') {
      p++;
      if (!skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
      continue;
    }
    if (*p == ']') {
      p++;
      if (finish_ws_only(p, end) != SUCCESS) { out.clear(); return UNINITIALIZED; }
      return SUCCESS;
    }
    out.clear();
    return UNINITIALIZED;
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

// ---- schema-directed struct parse (simdjson::direct_schema<T>) ----
//
// Recursive descent over plain ASCII JSON. No padding, no structural
// index. A byte >= 0x80 or any shape we will not fully accept yields
// UNINITIALIZED so the caller falls back to On Demand.

inline constexpr int schema_max_depth = 256;

template <typename T>
inline constexpr bool has_direct_schema_v = simdjson::direct_schema<T>::enabled;

template <typename T> struct is_std_vector : std::false_type {};
template <typename U, typename A> struct is_std_vector<std::vector<U, A>> : std::true_type {};

template <typename T>
constexpr bool schema_document_v_impl() {
  if constexpr (is_std_vector<T>::value) {
    return has_direct_schema_v<typename T::value_type>;
  } else {
    return has_direct_schema_v<T>;
  }
}
template <typename T>
inline constexpr bool schema_document_v = schema_document_v_impl<T>();

template <typename T>
inline constexpr bool is_direct_char_v =
    std::is_same_v<T, char> || std::is_same_v<T, signed char> ||
    std::is_same_v<T, unsigned char> || std::is_same_v<T, wchar_t> ||
    std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>
#if SIMDJSON_SUPPORTS_CHAR8_T
    || std::is_same_v<T, char8_t>
#endif
    ;

// Validate and consume one JSON number. On success p is at the first
// byte after the number and num_begin points at its first byte.
simdjson_inline error_code consume_json_number(const uint8_t *&p, const uint8_t *end,
                                              const uint8_t *&num_begin) noexcept {
  if (p >= end || *p >= 0x80) { return UNINITIALIZED; }
  num_begin = p;
  if (*p == '-') {
    p++;
    if (p >= end) { return UNINITIALIZED; }
  }
  if (*p < '0' || *p > '9') { return UNINITIALIZED; }
  if (*p == '0') {
    p++;
  } else {
    while (p < end && *p >= '0' && *p <= '9') { p++; }
  }
  if (p < end && *p == '.') {
    p++;
    if (p >= end || *p < '0' || *p > '9') { return UNINITIALIZED; }
    while (p < end && *p >= '0' && *p <= '9') { p++; }
  }
  if (p < end && (*p == 'e' || *p == 'E')) {
    p++;
    if (p < end && (*p == '+' || *p == '-')) { p++; }
    if (p >= end || *p < '0' || *p > '9') { return UNINITIALIZED; }
    while (p < end && *p >= '0' && *p <= '9') { p++; }
  }
  if (p < end && *p >= 0x80) { return UNINITIALIZED; }
  return SUCCESS;
}

// Clinger fast path: |exponent| <= 22 and mantissa < 2^53 are exact in
// binary64, so one multiply or divide by 10^|e| is correctly rounded.
// Longer numbers go through from_chars.
simdjson_really_inline error_code parse_double_token(const uint8_t *&p, const uint8_t *end, double &out) noexcept {
  if (p >= end || *p >= 0x80) { return UNINITIALIZED; }
  const uint8_t *save = p;
  const bool negative = (*p == '-');
  if (negative) {
    p++;
    if (p >= end) { p = save; return UNINITIALIZED; }
  }
  if (*p < '0' || *p > '9') { p = save; return UNINITIALIZED; }
  uint64_t i = 0;
  int digits = 0;
  int frac = 0;
  if (*p == '0') {
    p++;
    digits = 1;
    if (p < end && *p >= '0' && *p <= '9') { p = save; return UNINITIALIZED; }
  } else {
    while (p < end && *p >= '0' && *p <= '9') {
      if (digits < 19) { i = i * 10 + uint64_t(*p - '0'); }
      digits++;
      p++;
    }
  }
  if (p < end && *p == '.') {
    p++;
    if (p >= end || *p < '0' || *p > '9') { p = save; return UNINITIALIZED; }
    while (p < end && *p >= '0' && *p <= '9') {
      if (digits < 19) { i = i * 10 + uint64_t(*p - '0'); }
      digits++;
      frac++;
      p++;
    }
  }
  int64_t exponent = -int64_t(frac);
  bool exp_overflow = false;
  if (p < end && (*p == 'e' || *p == 'E')) {
    p++;
    bool eneg = false;
    if (p < end && (*p == '+' || *p == '-')) { eneg = (*p == '-'); p++; }
    if (p >= end || *p < '0' || *p > '9') { p = save; return UNINITIALIZED; }
    int64_t expv = 0;
    int ed = 0;
    while (p < end && *p >= '0' && *p <= '9') {
      if (ed < 9) { expv = expv * 10 + (*p - '0'); }
      ed++;
      p++;
    }
    if (ed > 9) { exp_overflow = true; }
    exponent += eneg ? -expv : expv;
  }
  if (p < end && *p >= 0x80) { p = save; return UNINITIALIZED; }
  if (!exp_overflow && digits <= 19 && i <= 9007199254740991ull &&
      exponent >= -22 && exponent <= 22) {
    static constexpr double pow10[23] = {
      1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,
      1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
      1e20, 1e21, 1e22
    };
    double d = double(i);
    if (exponent < 0) { d = d / pow10[int(-exponent)]; }
    else { d = d * pow10[int(exponent)]; }
    out = negative ? -d : d;
    return SUCCESS;
  }
  double d = simdjson::internal::from_chars(reinterpret_cast<const char *>(save),
                                            reinterpret_cast<const char *>(p));
  if (!std::isfinite(d)) { p = save; return UNINITIALIZED; }
  out = d;
  return SUCCESS;
}

// String including escapes, contents not materialized. Grammar only.
simdjson_inline error_code skip_json_string(const uint8_t *&p, const uint8_t *end) noexcept {
  if (p >= end || *p != '"') { return UNINITIALIZED; }
  p++;
  while (p < end) {
    uint8_t c = *p++;
    if (c == '"') { return SUCCESS; }
    if (c < 0x20 || c >= 0x80) { return UNINITIALIZED; }
    if (c == '\\') {
      if (p >= end) { return UNINITIALIZED; }
      uint8_t e = *p++;
      switch (e) {
      case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't':
        break;
      case 'u': {
        if (size_t(end - p) < 4) { return UNINITIALIZED; }
        for (int i = 0; i < 4; i++) {
          uint8_t h = p[i];
          bool hex = (h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F');
          if (!hex) { return UNINITIALIZED; }
        }
        p += 4;
        break;
      }
      default:
        return UNINITIALIZED;
      }
    }
  }
  return UNINITIALIZED;
}

// Plain key: no escapes, no controls. key points into the buffer.
simdjson_inline error_code read_plain_key(const uint8_t *&p, const uint8_t *end,
                                          const uint8_t *&key, size_t &key_len) noexcept {
  if (p >= end || *p != '"') { return UNINITIALIZED; }
  p++;
  key = p;
  while (p < end) {
    uint8_t c = *p;
    if (c == '"') {
      key_len = size_t(p - key);
      p++;
      return SUCCESS;
    }
    if (c == '\\' || c < 0x20 || c >= 0x80) { return UNINITIALIZED; }
    p++;
  }
  return UNINITIALIZED;
}

simdjson_inline error_code skip_value(const uint8_t *&p, const uint8_t *end, int depth) noexcept;
simdjson_inline error_code skip_object(const uint8_t *&p, const uint8_t *end, int depth) noexcept;
simdjson_inline error_code skip_array(const uint8_t *&p, const uint8_t *end, int depth) noexcept;

simdjson_inline error_code expect_colon(const uint8_t *&p, const uint8_t *end) noexcept {
  if (!skip_ws(p, end) || p >= end || *p != ':') { return UNINITIALIZED; }
  p++;
  return SUCCESS;
}

simdjson_inline error_code skip_object(const uint8_t *&p, const uint8_t *end, int depth) noexcept {
  if (depth > schema_max_depth) { return UNINITIALIZED; }
  if (p >= end || *p != '{') { return UNINITIALIZED; }
  p++;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p < end && *p == '}') { p++; return SUCCESS; }
  for (;;) {
    if (skip_json_string(p, end) != SUCCESS) { return UNINITIALIZED; }
    if (expect_colon(p, end) != SUCCESS) { return UNINITIALIZED; }
    if (skip_value(p, end, depth + 1) != SUCCESS) { return UNINITIALIZED; }
    if (!skip_ws(p, end) || p >= end) { return UNINITIALIZED; }
    if (*p == ',') {
      p++;
      if (!skip_ws(p, end)) { return UNINITIALIZED; }
      continue;
    }
    if (*p == '}') { p++; return SUCCESS; }
    return UNINITIALIZED;
  }
}

simdjson_inline error_code skip_array(const uint8_t *&p, const uint8_t *end, int depth) noexcept {
  if (depth > schema_max_depth) { return UNINITIALIZED; }
  if (p >= end || *p != '[') { return UNINITIALIZED; }
  p++;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p < end && *p == ']') { p++; return SUCCESS; }
  for (;;) {
    if (skip_value(p, end, depth + 1) != SUCCESS) { return UNINITIALIZED; }
    if (!skip_ws(p, end) || p >= end) { return UNINITIALIZED; }
    if (*p == ',') {
      p++;
      if (!skip_ws(p, end)) { return UNINITIALIZED; }
      continue;
    }
    if (*p == ']') { p++; return SUCCESS; }
    return UNINITIALIZED;
  }
}

simdjson_inline error_code skip_value(const uint8_t *&p, const uint8_t *end, int depth) noexcept {
  if (!skip_ws(p, end) || p >= end) { return UNINITIALIZED; }
  if (depth > schema_max_depth) { return UNINITIALIZED; }
  uint8_t c = *p;
  if (c == '"') { return skip_json_string(p, end); }
  if (c == '{') { return skip_object(p, end, depth); }
  if (c == '[') { return skip_array(p, end, depth); }
  if (c == 't' || c == 'f') {
    bool b;
    return parse_bool_token(p, end, b);
  }
  if (c == 'n') { return parse_null_token(p, end); }
  if (c == '-' || (c >= '0' && c <= '9')) {
    const uint8_t *begin;
    return consume_json_number(p, end, begin);
  }
  return UNINITIALIZED;
}

template <typename M>
error_code parse_schema_value(const uint8_t *&p, const uint8_t *end, M &out, int depth);

template <typename T, typename = void>
struct schema_has_fields : std::false_type {};
template <typename T>
struct schema_has_fields<T, std::void_t<decltype(simdjson::direct_schema<T>::fields)>> : std::true_type {};

template <typename T, typename = void>
struct schema_has_for_each : std::false_type {};
template <typename T>
struct schema_has_for_each<T, std::void_t<decltype(simdjson::direct_schema<T>::for_each_field)>> : std::true_type {};
// for_each_field is a function template, so decltype(for_each_field) may fail.
// Detect via a pointer-to-template is hard; try a call with a dummy lambda
// in a separate trait below.

template <typename Schema, typename = void>
struct schema_for_each_callable : std::false_type {};
template <typename Schema>
struct schema_for_each_callable<Schema, std::void_t<decltype(
  Schema::for_each_field([](std::string_view, auto) {})
)>> : std::true_type {};

// Parse fields[Idx..] in order. Returns SUCCESS if every remaining field
// matched; UNINITIALIZED if the shape does not match the ordered pattern.
template <typename T, size_t Idx>
simdjson_really_inline error_code parse_ordered_fields(const uint8_t *&p, const uint8_t *end, T &out, int depth) {
  using fields_t = decltype(simdjson::direct_schema<T>::fields);
  constexpr size_t N = std::tuple_size_v<fields_t>;
  if constexpr (Idx >= N) {
    return SUCCESS;
  } else {
    if constexpr (Idx > 0) {
      if (p < end && *p == ',') { p++; }
      else {
        if (!skip_ws(p, end) || p >= end || *p != ',') { return UNINITIALIZED; }
        p++;
        if (!skip_ws(p, end)) { return UNINITIALIZED; }
      }
    }
    using field = std::tuple_element_t<Idx, fields_t>;
    constexpr auto name = field::name;
    constexpr size_t n = name.size();
    if (size_t(end - p) < n + 2 || p[0] != '"' ||
        std::memcmp(p + 1, name.data(), n) != 0 || p[1 + n] != '"') {
      return UNINITIALIZED;
    }
    p += n + 2;
    if (p < end && *p == ':') { p++; }
    else if (expect_colon(p, end) != SUCCESS) { return UNINITIALIZED; }
    error_code err = parse_schema_value(p, end, out.*(field::member), depth + 1);
    if (err != SUCCESS) { return err; }
    return parse_ordered_fields<T, Idx + 1>(p, end, out, depth);
  }
}

template <typename T, size_t Idx>
simdjson_really_inline error_code match_field_by_index(const uint8_t *key, size_t key_len,
                                                      const uint8_t *&p, const uint8_t *end,
                                                      T &out, int depth, bool &matched) {
  using fields_t = decltype(simdjson::direct_schema<T>::fields);
  constexpr size_t N = std::tuple_size_v<fields_t>;
  if constexpr (Idx >= N) {
    return SUCCESS;
  } else {
    if (!matched) {
      using field = std::tuple_element_t<Idx, fields_t>;
      constexpr auto name = field::name;
      if (name.size() == key_len && std::memcmp(name.data(), key, key_len) == 0) {
        matched = true;
        return parse_schema_value(p, end, out.*(field::member), depth + 1);
      }
    }
    return match_field_by_index<T, Idx + 1>(key, key_len, p, end, out, depth, matched);
  }
}

// General (any key order) object body. Outlined so the in-order path
// stays a small inline function, the same split Jsonifier uses
// (impl vs implOutline). `p` is at the first key.
template <typename T>
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
error_code parse_schema_object_general(const uint8_t *&p, const uint8_t *end, T &out, int depth) {
  for (;;) {
    const uint8_t *key;
    size_t key_len;
    if (read_plain_key(p, end, key, key_len) != SUCCESS) { return UNINITIALIZED; }
    if (expect_colon(p, end) != SUCCESS) { return UNINITIALIZED; }
    if (!skip_ws(p, end)) { return UNINITIALIZED; }
    bool matched = false;
    error_code err = SUCCESS;
    if constexpr (schema_has_fields<T>::value) {
      err = match_field_by_index<T, 0>(key, key_len, p, end, out, depth, matched);
    } else {
      simdjson::direct_schema<T>::for_each_field([&](std::string_view name, auto mem) {
        if (matched || err != SUCCESS) { return; }
        if (name.size() != key_len) { return; }
        if (std::memcmp(name.data(), key, key_len) != 0) { return; }
        matched = true;
        err = parse_schema_value(p, end, out.*mem, depth + 1);
      });
    }
    if (err != SUCCESS) { return err; }
    if (!matched) {
      if (skip_value(p, end, depth + 1) != SUCCESS) { return UNINITIALIZED; }
    }
    if (!skip_ws(p, end) || p >= end) { return UNINITIALIZED; }
    if (*p == ',') {
      p++;
      if (!skip_ws(p, end)) { return UNINITIALIZED; }
      // trailing comma
      if (p < end && *p == '}') { return UNINITIALIZED; }
      continue;
    }
    if (*p == '}') { p++; return SUCCESS; }
    return UNINITIALIZED;
  }
}

template <typename T>
simdjson_really_inline error_code parse_schema_object(const uint8_t *&p, const uint8_t *end, T &out, int depth) {
  if (depth > schema_max_depth) { return UNINITIALIZED; }
  if (!skip_ws(p, end) || p >= end || *p != '{') { return UNINITIALIZED; }
  p++;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p < end && *p == '}') { p++; return SUCCESS; }
  // In-order fast path: keys in schema order, no unknown keys.
  // Mismatch rewinds to the first key and uses the general scanner.
  const uint8_t *first = p;
  error_code ord = SUCCESS;
  bool ordered_ok = true;
  if constexpr (schema_has_fields<T>::value) {
    ord = parse_ordered_fields<T, 0>(p, end, out, depth);
    if (ord != SUCCESS) { ordered_ok = false; }
  } else {
    size_t idx = 0;
    simdjson::direct_schema<T>::for_each_field([&](std::string_view name, auto mem) {
      if (!ordered_ok || ord != SUCCESS) { return; }
      if (idx > 0) {
        if (p < end && *p == ',') { p++; }
        else {
          if (!skip_ws(p, end) || p >= end || *p != ',') { ordered_ok = false; return; }
          p++;
          if (!skip_ws(p, end)) { ordered_ok = false; return; }
        }
      }
      idx++;
      const size_t n = name.size();
      if (size_t(end - p) < n + 2 || p[0] != '"' ||
          std::memcmp(p + 1, name.data(), n) != 0 || p[1 + n] != '"') {
        ordered_ok = false;
        return;
      }
      p += n + 2;
      if (p < end && *p == ':') { p++; }
      else if (expect_colon(p, end) != SUCCESS) { ordered_ok = false; return; }
      ord = parse_schema_value(p, end, out.*mem, depth + 1);
    });
  }
  if (ordered_ok && ord == SUCCESS && p < end && *p == '}') {
    p++;
    return SUCCESS;
  }
  if (ordered_ok && ord == SUCCESS) {
    if (skip_ws(p, end) && p < end && *p == '}') { p++; return SUCCESS; }
  }
  out = T{};
  p = first;
  return parse_schema_object_general(p, end, out, depth);
}

// Flat array of doubles. Small arrays (the common struct-field case)
// are parsed into a stack buffer and assigned once, so a 4-element
// tire-pressure field is a single exact allocation instead of a
// geometric grow plus a parse_schema_value call per element.
simdjson_really_inline error_code parse_double_array(const uint8_t *&p, const uint8_t *end,
                                                    std::vector<double> &out, int depth) {
  if (depth > schema_max_depth) { return UNINITIALIZED; }
  if (p < end && is_ws(*p)) {
    if (!skip_ws(p, end)) { return UNINITIALIZED; }
  }
  if (p >= end || *p != '[') { return UNINITIALIZED; }
  p++;
  if (p < end && is_ws(*p)) {
    if (!skip_ws(p, end)) { return UNINITIALIZED; }
  }
  if (p < end && *p == ']') { out.clear(); p++; return SUCCESS; }
  constexpr size_t kStack = 16;
  double stack[kStack];
  size_t n = 0;
  bool spilled = false;
  for (;;) {
    double v;
    // Minified arrays have no space before the number. skip_ws only
    // when the next byte is actually whitespace.
    if (p < end && is_ws(*p)) {
      if (!skip_ws(p, end) || p >= end) { out.clear(); return UNINITIALIZED; }
    }
    if (parse_double_token(p, end, v) != SUCCESS) { out.clear(); return UNINITIALIZED; }
    if (!spilled) {
      if (n < kStack) {
        stack[n++] = v;
      } else {
        out.assign(stack, stack + kStack);
        out.push_back(v);
        n++;
        spilled = true;
      }
    } else {
      out.push_back(v);
      n++;
    }
    if (p < end && *p == ',') {
      p++;
      continue;
    }
    if (p < end && is_ws(*p)) {
      if (!skip_ws(p, end) || p >= end) { out.clear(); return UNINITIALIZED; }
      if (*p == ',') {
        p++;
        if (p < end && is_ws(*p) && !skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
        if (p < end && *p == ']') { out.clear(); return UNINITIALIZED; }
        continue;
      }
    }
    if (p < end && *p == ']') {
      p++;
      if (!spilled) { out.assign(stack, stack + n); }
      return SUCCESS;
    }
    out.clear();
    return UNINITIALIZED;
  }
}

template <typename Vec>
simdjson_really_inline error_code parse_schema_array(const uint8_t *&p, const uint8_t *end, Vec &out, int depth) {
  if (depth > schema_max_depth) { return UNINITIALIZED; }
  if (!skip_ws(p, end) || p >= end || *p != '[') { return UNINITIALIZED; }
  p++;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  out.clear();
  if (p < end && *p == ']') { p++; return SUCCESS; }
  bool reserved = false;
  for (;;) {
    const uint8_t *tok = p;
    typename Vec::value_type elem{};
    error_code err = parse_schema_value(p, end, elem, depth + 1);
    if (err != SUCCESS) { out.clear(); return UNINITIALIZED; }
    if (!reserved) {
      // The byte-length heuristic is only valid when this array is the
      // document (depth 0): `end` is the end of the whole buffer, so a
      // nested array would reserve as if it ran to EOF (a 4-double
      // field would allocate tens of thousands of slots per object).
      if (depth == 0) {
        const size_t elem_bytes = size_t(p - tok) + 1;
        const size_t rest = size_t(end - tok);
        out.reserve(elem_bytes ? rest / elem_bytes + 2 : 8);
      } else {
        // Nested arrays: one small allocation covers the common
        // fixed-size fields (a handful of numbers) without the
        // 0->1->2->4 geometric dance. Larger arrays grow from here.
        out.reserve(8);
      }
      reserved = true;
    }
    out.push_back(std::move(elem));
    if (!skip_ws(p, end) || p >= end) { out.clear(); return UNINITIALIZED; }
    if (*p == ',') {
      p++;
      if (!skip_ws(p, end)) { out.clear(); return UNINITIALIZED; }
      if (p < end && *p == ']') { out.clear(); return UNINITIALIZED; }
      continue;
    }
    if (*p == ']') { p++; return SUCCESS; }
    out.clear();
    return UNINITIALIZED;
  }
}

template <typename M>
simdjson_really_inline error_code parse_schema_value(const uint8_t *&p, const uint8_t *end, M &out, int depth) {
  using D = std::remove_cv_t<M>;
  if (!skip_ws(p, end) || p >= end) { return UNINITIALIZED; }
  if constexpr (std::is_same_v<D, bool>) {
    return parse_bool_token(p, end, out);
  } else if constexpr (std::is_same_v<D, std::string>) {
    return parse_plain_string_token(p, end, out);
  } else if constexpr (std::is_same_v<D, double>) {
    return parse_double_token(p, end, out);
  } else if constexpr (std::is_integral_v<D> && !std::is_same_v<D, bool> && !is_direct_char_v<D>) {
    if constexpr (std::is_unsigned_v<D>) {
      // parse_uint64_token rejects values above INT64_MAX (it reuses the
      // int64 scanner). Wider uint64 magnitudes fall back.
      uint64_t u;
      error_code err = parse_uint64_token(p, end, u);
      if (err != SUCCESS) { return err; }
      if (u > uint64_t((std::numeric_limits<D>::max)())) { return UNINITIALIZED; }
      out = static_cast<D>(u);
      return SUCCESS;
    } else {
      int64_t v;
      error_code err = parse_int64_token(p, end, v);
      if (err != SUCCESS) { return err; }
      if (v < int64_t((std::numeric_limits<D>::min)()) ||
          v > int64_t((std::numeric_limits<D>::max)())) {
        return UNINITIALIZED;
      }
      out = static_cast<D>(v);
      return SUCCESS;
    }
  } else if constexpr (std::is_same_v<D, std::vector<double>>) {
    return parse_double_array(p, end, out, depth);
  } else if constexpr (is_std_vector<D>::value) {
    return parse_schema_array(p, end, out, depth);
  } else if constexpr (has_direct_schema_v<D>) {
    return parse_schema_object(p, end, out, depth);
  } else {
    // Unsupported field type: decline the whole document.
    return UNINITIALIZED;
  }
}

template <typename T>
error_code parse_schema_document(const uint8_t *buf, size_t len, T &out) {
  const uint8_t *p = buf;
  const uint8_t *end = buf + len;
  if (!skip_ws(p, end)) { return UNINITIALIZED; }
  if (p == end) { return EMPTY; }
  error_code err;
  if constexpr (is_std_vector<T>::value) {
    err = parse_schema_array(p, end, out, 0);
    if (err != SUCCESS) { out.clear(); return UNINITIALIZED; }
  } else {
    err = parse_schema_object(p, end, out, 0);
    if (err != SUCCESS) { return UNINITIALIZED; }
  }
  if (finish_ws_only(p, end) != SUCCESS) {
    if constexpr (is_std_vector<T>::value) { out.clear(); }
    return UNINITIALIZED;
  }
  return SUCCESS;
}

} // namespace direct
} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_DIRECT_PARSE_H
