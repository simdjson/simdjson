#ifndef SIMDJSON_GENERIC_ONDEMAND_PARSER_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_PARSER_INL_H
#include "simdjson/padded_string.h"
#include "simdjson/padded_string_view.h"
#include "simdjson/implementation.h"
#include "simdjson/internal/dom_parser_implementation.h"
#include "simdjson/dom/base.h" // for MINIMAL_DOCUMENT_CAPACITY
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/document_stream.h"
#include "simdjson/generic/ondemand/parser.h"
#include "simdjson/generic/ondemand/raw_json_string.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_inline parser::parser(size_t max_capacity) noexcept
  : _max_capacity{max_capacity} {
}

simdjson_warn_unused simdjson_inline error_code parser::allocate(size_t new_capacity, size_t new_max_depth) noexcept {
  // Reject an invalid depth before touching anything, as the implementation
  // would: a failed allocate() leaves the parser as it was.
  if (new_max_depth == 0 || new_max_depth > SIMDJSON_MAX_DEPTH) { return CAPACITY; }
  if (new_capacity > max_capacity()) { return CAPACITY; }
  if (string_buf && new_capacity <= capacity() && new_max_depth <= max_depth()) { return SUCCESS; }

  // string_capacity copied from document::allocate
  if (5 * (new_capacity / 3) + SIMDJSON_PADDING < SIMDJSON_PADDING) {
    return CAPACITY; // overflow, only happen on legacy 32-bit systems with very large capacity
  }
  size_t string_capacity = SIMDJSON_ROUNDUP_N(5 * (new_capacity / 3) + SIMDJSON_PADDING, 64);
  simdjson::internal::allocated_array<uint8_t> new_string_buf;
  if (!new_string_buf.allocate(string_capacity, _allocator)) { return MEMALLOC; }
#if SIMDJSON_DEVELOPMENT_CHECKS
  std::unique_ptr<token_position[]> new_start_positions(new (std::nothrow) token_position[new_max_depth]);
  if (!new_start_positions) { return MEMALLOC; }
#endif
  if (!implementation) {
    error_code err = simdjson::get_active_implementation()->create_dom_parser_implementation(0, new_max_depth, implementation);
    if (err) {
      implementation.reset();
      return err;
    }
  }
  implementation->allocator = _allocator;
  error_code err = implementation->allocate(new_capacity, new_max_depth);
  if (err) { return err; }
  string_buf = std::move(new_string_buf);
#if SIMDJSON_DEVELOPMENT_CHECKS
  start_positions = std::move(new_start_positions);
#endif
  _capacity = new_capacity;
  _max_depth = new_max_depth;
  return SUCCESS;
}
#if SIMDJSON_DEVELOPMENT_CHECKS
simdjson_inline simdjson_warn_unused bool parser::string_buffer_overflow(const uint8_t *string_buf_loc) const noexcept {
  return (string_buf_loc < string_buf.get()) || (size_t(string_buf_loc - string_buf.get()) >= capacity());
}
#endif

simdjson_warn_unused simdjson_inline simdjson_result<document> parser::iterate(padded_string_view json) & noexcept {
  if (!json.has_sufficient_padding()) { return INSUFFICIENT_PADDING; }
  json.use_trailing_whitespace_as_padding();

  json.remove_utf8_bom();
  _document_len = json.length();

  // Allocate if needed
  if (capacity() < json.length() || !string_buf) {
    SIMDJSON_TRY( allocate(json.length(), max_depth()) );
  }

  // Run stage 1.
  SIMDJSON_TRY( implementation->stage1(reinterpret_cast<const uint8_t *>(json.data()), json.length(), stage1_mode::regular) );
#if SIMDJSON_ALLOW_TRAILING_COMMAS
  drop_trailing_commas(reinterpret_cast<const uint8_t *>(json.data()));
#endif
  return document::start({ reinterpret_cast<const uint8_t *>(json.data()), this });
}

// Removes from the structural indexes each comma that follows a value and
// precedes a closing bracket, so that the iterator never sees it. The entries
// past the last index (stage 1 keeps up to three there) move with the others.
#if SIMDJSON_ALLOW_TRAILING_COMMAS
simdjson_inline void parser::drop_trailing_commas(const uint8_t *buf) noexcept {
  uint32_t *indexes = implementation->structural_indexes.get();
  const uint32_t n = implementation->n_structural_indexes;
  uint32_t kept = 0;
  uint8_t previous = '[';
  for (uint32_t i = 0; i < n; i++) {
    const uint8_t c = buf[indexes[i]];
    if (c == ',' && i + 1 < n && previous != '[' && previous != '{' && previous != ',' && previous != ':') {
      const uint8_t next = buf[indexes[i + 1]];
      if (next == ']' || next == '}') {
        previous = c;
        continue;
      }
    }
    indexes[kept++] = indexes[i];
    previous = c;
  }
  if (kept != n) {
    indexes[kept] = indexes[n];
    indexes[kept + 1] = indexes[n + 1];
    indexes[kept + 2] = indexes[n + 2];
    implementation->n_structural_indexes = kept;
  }
}
#endif // SIMDJSON_ALLOW_TRAILING_COMMAS

#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
simdjson_warn_unused simdjson_inline simdjson_result<document> parser::iterate_allow_incomplete_json(padded_string_view json) & noexcept {
  if (!json.has_sufficient_padding()) { return INSUFFICIENT_PADDING; }
  json.use_trailing_whitespace_as_padding();

  json.remove_utf8_bom();
  _document_len = json.length();

  // Allocate if needed
  if (capacity() < json.length() || !string_buf) {
    SIMDJSON_TRY( allocate(json.length(), max_depth()) );
  }

  // Run stage 1.
  const simdjson::error_code err = implementation->stage1(reinterpret_cast<const uint8_t *>(json.data()), json.length(), stage1_mode::regular);
  if (err) {
    if (err != UNCLOSED_STRING)
      return err;
  }
  SIMDJSON_TRY( end_incomplete_json(json) );
  const uint8_t *buf = reinterpret_cast<const uint8_t *>(json.data());
  const uint8_t first = buf[implementation->structural_indexes[0]];
  const bool lone_container = implementation->n_structural_indexes == 1 && (first == '{' || first == '[');
  return document::start({ buf, this, !lone_container });
}

simdjson_inline bool parser::can_end_incomplete_json(uint8_t c) noexcept {
  return c != '"' && c != ',' && c != ']' && c != '}';
}

simdjson_warn_unused simdjson_inline error_code parser::end_incomplete_json(padded_string_view json) noexcept {
  const uint8_t *buf = reinterpret_cast<const uint8_t *>(json.data());
  const size_t len = json.length();
  uint32_t *indexes = implementation->structural_indexes.get();
  uint32_t n = implementation->n_structural_indexes;
  while (n > 1) {
    const uint8_t last = buf[indexes[n - 1]];
    if (last != ',' && last != ':' && last != '[' && last != '{') { break; }
    n--;
  }
  implementation->n_structural_indexes = n;
  indexes[n] = uint32_t(len);
  indexes[n + 1] = uint32_t(len);
  indexes[n + 2] = 0;
  const uint8_t first = buf[indexes[0]];
  if (n == 1 && first != '{' && first != '[') { return SUCCESS; }
  if (simdjson_likely(can_end_incomplete_json(buf[len]))) { return SUCCESS; }
  size_t end = len + 1;
  while (end < len + SIMDJSON_PADDING && !can_end_incomplete_json(buf[end])) { end++; }
  if (end == len + SIMDJSON_PADDING) { return INSUFFICIENT_PADDING; }
  indexes[n] = uint32_t(end);
  indexes[n + 1] = uint32_t(end);
  return SUCCESS;
}
#endif // SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON

simdjson_warn_unused simdjson_inline simdjson_result<document> parser::iterate(const char *json, size_t len, size_t allocated) & noexcept {
  return iterate(padded_string_view(json, len, allocated));
}

simdjson_warn_unused simdjson_inline simdjson_result<document> parser::iterate(const uint8_t *json, size_t len, size_t allocated) & noexcept {
  return iterate(padded_string_view(json, len, allocated));
}

simdjson_warn_unused simdjson_inline simdjson_result<document> parser::iterate(std::string_view json, size_t allocated) & noexcept {
  return iterate(padded_string_view(json, allocated));
}

simdjson_warn_unused simdjson_inline simdjson_result<document> parser::iterate(std::string &json) & noexcept {
  return iterate(pad_with_reserve(json));
}

simdjson_warn_unused simdjson_inline simdjson_result<document> parser::iterate(const std::string &json) & noexcept {
  return iterate(padded_string_view(json));
}

simdjson_warn_unused simdjson_inline simdjson_result<document> parser::iterate(const simdjson_result<padded_string_view> &result) & noexcept {
  // We don't presently have a way to temporarily get a const T& from a simdjson_result<T> without throwing an exception
  SIMDJSON_TRY( result.error() );
  padded_string_view json = result.value_unsafe();
  return iterate(json);
}

simdjson_warn_unused simdjson_inline simdjson_result<document> parser::iterate(const simdjson_result<padded_string> &result) & noexcept {
  // We don't presently have a way to temporarily get a const T& from a simdjson_result<T> without throwing an exception
  SIMDJSON_TRY( result.error() );
  const padded_string &json = result.value_unsafe();
  return iterate(json);
}

simdjson_warn_unused simdjson_inline simdjson_result<json_iterator> parser::iterate_raw(padded_string_view json) & noexcept {
  if (!json.has_sufficient_padding()) { return INSUFFICIENT_PADDING; }
  json.use_trailing_whitespace_as_padding();

  json.remove_utf8_bom();

  // Allocate if needed
  if (capacity() < json.length()) {
    SIMDJSON_TRY( allocate(json.length(), max_depth()) );
  }

  // Run stage 1.
  SIMDJSON_TRY( implementation->stage1(reinterpret_cast<const uint8_t *>(json.data()), json.length(), stage1_mode::regular) );
#if SIMDJSON_ALLOW_TRAILING_COMMAS
  drop_trailing_commas(reinterpret_cast<const uint8_t *>(json.data()));
#endif
  return json_iterator(reinterpret_cast<const uint8_t *>(json.data()), this);
}

namespace {
struct ignoring_visitor {
  simdjson_inline error_code on_object_start() { return SUCCESS; }
  simdjson_inline error_code on_object_end(uint32_t) { return SUCCESS; }
  simdjson_inline error_code on_array_start() { return SUCCESS; }
  simdjson_inline error_code on_array_end(uint32_t) { return SUCCESS; }
  simdjson_inline error_code on_key(std::string_view) { return SUCCESS; }
  simdjson_inline error_code on_string(std::string_view) { return SUCCESS; }
  simdjson_inline error_code on_int64(int64_t, const char *) { return SUCCESS; }
  simdjson_inline error_code on_uint64(uint64_t, const char *) { return SUCCESS; }
  simdjson_inline error_code on_double(double, const char *) { return SUCCESS; }
  simdjson_inline error_code on_bool(bool) { return SUCCESS; }
  simdjson_inline error_code on_null() { return SUCCESS; }
};
} // unnamed namespace

template <typename V>
simdjson_warn_unused simdjson_inline error_code parser::visit(padded_string_view json, V &visitor) & {
#if !SIMDJSON_ENABLE_NAN_INF && !SIMDJSON_ALLOW_TRAILING_COMMAS
  if (json.length() <= MAX_ONE_PASS_VISIT && json.has_sufficient_padding()) {
    if (capacity() < json.length() || !string_buf) {
      SIMDJSON_TRY( allocate(json.length(), max_depth()) );
    }
    bool visitor_failed = false;
    const error_code error = visit_one_pass(reinterpret_cast<const uint8_t *>(json.data()), json.length(), visitor, visitor_failed);
    if (error != UNINITIALIZED) {
      if (error != TAPE_ERROR || visitor_failed) { return error; }
      // The visitor has already seen part of the document: the regular walk only finds the
      // error code, and never calls the visitor a second time.
      document doc;
      SIMDJSON_TRY( iterate(json).get(doc) );
      ignoring_visitor ignoring;
      const error_code regular_error = doc.visit(ignoring);
      return regular_error ? regular_error : TAPE_ERROR;
    }
  }
#endif
  document doc;
  SIMDJSON_TRY( iterate(json).get(doc) );
  return doc.visit(visitor);
}

// A single pass over the bytes, for small documents. The root must be an array or an object, and
// the last byte that is not whitespace must be ] or }: before it, runs of whitespace, digits or
// atoms stop without a bound check. Only a closing bracket or a string may reach the end.
template <typename V>
simdjson_warn_unused simdjson_inline error_code parser::visit_one_pass(const uint8_t *const buf, const size_t len, V &visitor, bool &visitor_failed) {
  const auto is_whitespace = [](uint8_t c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; };
  if (len == 0 || len > MAX_ONE_PASS_VISIT) { return UNINITIALIZED; }
  const uint8_t *p = buf;
  const uint8_t *last = buf + len - 1;
  while (p < last && is_whitespace(*p)) { p++; }
  while (last > p && is_whitespace(*last)) { last--; }
  if ((*p != '{' && *p != '[') || (*last != '}' && *last != ']')) { return UNINITIALIZED; }
  const uint8_t *const end = last + 1;
  uint8_t *dst = string_buf.get();
  bool non_ascii = false;
  // For each open container, the number of its elements so far, with the top bit set for an
  // array. Each container opens at a distinct byte before the last one, so that the depth stays
  // below MAX_ONE_PASS_VISIT.
  constexpr uint32_t ARRAY_BIT = uint32_t(1) << 31;
  uint32_t stack[MAX_ONE_PASS_VISIT];
  size_t depth = 0;
  uint8_t c = *p;

#define SIMDJSON_ONE_PASS_CALL(EXPR) \
  do { \
    const error_code one_pass_error = (EXPR); \
    if (simdjson_unlikely(one_pass_error != SUCCESS)) { visitor_failed = true; return one_pass_error; } \
  } while (0)
#define SIMDJSON_ONE_PASS_WHITESPACE() while (is_whitespace(*p)) { p++; }
// The string at p, which is left past the closing quote.
#define SIMDJSON_ONE_PASS_STRING(METHOD) \
  do { \
    const uint8_t *const start = ++p; \
    bool escaped = false; \
    for (;; p++) { \
      if (simdjson_unlikely(p >= end)) { return TAPE_ERROR; } \
      if (*p == '"') { break; } \
      if (*p == '\\') { \
        escaped = true; \
        p++; \
      } else if (simdjson_unlikely(*p < 0x20)) { \
        return TAPE_ERROR; \
      } else if (*p >= 0x80) { \
        non_ascii = true; \
      } \
    } \
    std::string_view text(reinterpret_cast<const char *>(start), size_t(p - start)); \
    p++; \
    if (escaped) { \
      uint8_t *const unescaped_end = implementation->parse_string(start, dst, false); \
      if (unescaped_end == nullptr) { return TAPE_ERROR; } \
      text = std::string_view(reinterpret_cast<const char *>(dst), size_t(unescaped_end - dst)); \
      dst = unescaped_end; \
    } \
    SIMDJSON_ONE_PASS_CALL(visitor.METHOD(text)); \
  } while (0)

  goto container_start;

value:
  switch (*p) {
    case '{': case '[':
      c = *p;
      goto container_start;
    case '"':
      SIMDJSON_ONE_PASS_STRING(on_string);
      goto value_end;
    case 't':
      if (!atomparsing::is_valid_true_atom(p)) { return TAPE_ERROR; }
      SIMDJSON_ONE_PASS_CALL(visitor.on_bool(true));
      p += 4;
      goto value_end;
    case 'f':
      if (!atomparsing::is_valid_false_atom(p)) { return TAPE_ERROR; }
      SIMDJSON_ONE_PASS_CALL(visitor.on_bool(false));
      p += 5;
      goto value_end;
    case 'n':
      if (!atomparsing::is_valid_null_atom(p)) { return TAPE_ERROR; }
      SIMDJSON_ONE_PASS_CALL(visitor.on_null());
      p += 4;
      goto value_end;
    default: {
      number value;
      if (numberparsing::parse_number(p, value) != SUCCESS) { return TAPE_ERROR; }
      SIMDJSON_ONE_PASS_CALL(json_iterator::visit_number(value, reinterpret_cast<const char *>(p), visitor));
      while (jsoncharutils::is_not_structural_or_whitespace(*p)) { p++; }
      goto value_end;
    }
  }

container_start:
  p++;
  SIMDJSON_ONE_PASS_WHITESPACE();
  // As in DOM, an empty container does not count toward the maximal depth.
  if (*p == c + 2) {
    p++;
    if (c == '[') {
      SIMDJSON_ONE_PASS_CALL(visitor.on_array_start());
      SIMDJSON_ONE_PASS_CALL(visitor.on_array_end(0));
    } else {
      SIMDJSON_ONE_PASS_CALL(visitor.on_object_start());
      SIMDJSON_ONE_PASS_CALL(visitor.on_object_end(0));
    }
    if (depth == 0) { goto document_end; }
    if (p == end) { return TAPE_ERROR; }
    goto value_end;
  }
  if (++depth >= max_depth()) { return TAPE_ERROR; }
  stack[depth] = 1;
  if (c == '[') {
    stack[depth] |= ARRAY_BIT;
    SIMDJSON_ONE_PASS_CALL(visitor.on_array_start());
    goto value;
  }
  SIMDJSON_ONE_PASS_CALL(visitor.on_object_start());

key:
  if (*p != '"') { return TAPE_ERROR; }
  SIMDJSON_ONE_PASS_STRING(on_key);
  SIMDJSON_ONE_PASS_WHITESPACE();
  if (*p != ':') { return TAPE_ERROR; }
  p++;
  SIMDJSON_ONE_PASS_WHITESPACE();
  goto value;

value_end:
  SIMDJSON_ONE_PASS_WHITESPACE();
  c = *p++;
  if (c == ',') {
    stack[depth]++;
    SIMDJSON_ONE_PASS_WHITESPACE();
    if (stack[depth] & ARRAY_BIT) { goto value; }
    goto key;
  }
  if (stack[depth] & ARRAY_BIT) {
    if (c != ']') { return TAPE_ERROR; }
    SIMDJSON_ONE_PASS_CALL(visitor.on_array_end(stack[depth] & ~ARRAY_BIT));
  } else {
    if (c != '}') { return TAPE_ERROR; }
    SIMDJSON_ONE_PASS_CALL(visitor.on_object_end(stack[depth]));
  }
  if (--depth > 0) {
    if (p == end) { return TAPE_ERROR; }
    goto value_end;
  }

document_end:
#undef SIMDJSON_ONE_PASS_CALL
#undef SIMDJSON_ONE_PASS_WHITESPACE
#undef SIMDJSON_ONE_PASS_STRING
  if (p != end) { return TAPE_ERROR; }
  if (non_ascii && !simdjson::validate_utf8(reinterpret_cast<const char *>(buf), len)) { return TAPE_ERROR; }
  return SUCCESS;
}

inline simdjson_result<document_stream> parser::iterate_many(const uint8_t *buf, size_t len, size_t batch_size) noexcept {
  if(batch_size < MINIMAL_BATCH_SIZE) { batch_size = MINIMAL_BATCH_SIZE; }
  if((len >= 3) && (std::memcmp(buf, "\xEF\xBB\xBF", 3) == 0)) {
    buf += 3;
    len -= 3;
  }
  return document_stream(*this, buf, len, batch_size, false, stream_format::whitespace_delimited);
}
inline simdjson_result<document_stream> parser::iterate_many(const char *buf, size_t len, size_t batch_size) noexcept {
  return iterate_many(reinterpret_cast<const uint8_t *>(buf), len, batch_size);
}
inline simdjson_result<document_stream> parser::iterate_many(padded_string_view s, size_t batch_size) noexcept {
  if (!s.has_sufficient_padding()) { return INSUFFICIENT_PADDING; }
  s.use_trailing_whitespace_as_padding();
  return iterate_many(s.data(), s.length(), batch_size);
}
inline simdjson_result<document_stream> parser::iterate_many(const padded_string &s, size_t batch_size) noexcept {
  return iterate_many(padded_string_view(s), batch_size);
}
inline simdjson_result<document_stream> parser::iterate_many(const std::string &s, size_t batch_size) noexcept {
  return iterate_many(padded_string_view(s), batch_size);
}
inline simdjson_result<document_stream> parser::iterate_many(std::string &s, size_t batch_size) noexcept {
  return iterate_many(pad(s), batch_size);
}

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
inline simdjson_result<document_stream> parser::iterate_many(const uint8_t *buf, size_t len, size_t batch_size, bool allow_comma_separated) noexcept {
  // Warning: no check is done on the buffer padding. We trust the user.
  if(batch_size < MINIMAL_BATCH_SIZE) { batch_size = MINIMAL_BATCH_SIZE; }
  if((len >= 3) && (std::memcmp(buf, "\xEF\xBB\xBF", 3) == 0)) {
    buf += 3;
    len -= 3;
  }
  // Map allow_comma_separated to stream_format::comma_delimited
  if (allow_comma_separated) {
    return document_stream(*this, buf, len, batch_size, false, stream_format::comma_delimited);
  }
  return document_stream(*this, buf, len, batch_size, false, stream_format::whitespace_delimited);
}

inline simdjson_result<document_stream> parser::iterate_many(const char *buf, size_t len, size_t batch_size, bool allow_comma_separated) noexcept {
  // Warning: no check is done on the buffer padding. We trust the user.
  return iterate_many(reinterpret_cast<const uint8_t *>(buf), len, batch_size, allow_comma_separated);
}
inline simdjson_result<document_stream> parser::iterate_many(padded_string_view s, size_t batch_size, bool allow_comma_separated) noexcept {
  if (!s.has_sufficient_padding()) { return INSUFFICIENT_PADDING; }
  s.use_trailing_whitespace_as_padding();
  return iterate_many(s.data(), s.length(), batch_size, allow_comma_separated);
}
inline simdjson_result<document_stream> parser::iterate_many(const padded_string &s, size_t batch_size, bool allow_comma_separated) noexcept {
  return iterate_many(padded_string_view(s), batch_size, allow_comma_separated);
}
inline simdjson_result<document_stream> parser::iterate_many(const std::string &s, size_t batch_size, bool allow_comma_separated) noexcept {
  return iterate_many(padded_string_view(s), batch_size, allow_comma_separated);
}
inline simdjson_result<document_stream> parser::iterate_many(std::string &s, size_t batch_size, bool allow_comma_separated) noexcept {
  return iterate_many(pad(s), batch_size, allow_comma_separated);
}
SIMDJSON_POP_DISABLE_WARNINGS
#endif // SIMDJSON_DISABLE_DEPRECATED_API

inline simdjson_result<document_stream> parser::iterate_many(const uint8_t *buf, size_t len, size_t batch_size, stream_format format) noexcept {
  if(batch_size < MINIMAL_BATCH_SIZE) { batch_size = MINIMAL_BATCH_SIZE; }
  if((len >= 3) && (std::memcmp(buf, "\xEF\xBB\xBF", 3) == 0)) {
    buf += 3;
    len -= 3;
  }
  if (format == stream_format::comma_delimited_array) {
    // Strip leading JSON whitespace.
    while (len > 0 && (buf[0] == ' ' || buf[0] == '\t' || buf[0] == '\n' || buf[0] == '\r')) {
      buf++; len--;
    }
    // Expect the opening '['.
    if (len == 0 || buf[0] != '[') { return TAPE_ERROR; }
    buf++; len--;
    // Strip trailing JSON whitespace.
    while (len > 0 && (buf[len-1] == ' ' || buf[len-1] == '\t' || buf[len-1] == '\n' || buf[len-1] == '\r')) {
      len--;
    }
    // Expect the closing ']'.
    if (len == 0 || buf[len-1] != ']') { return TAPE_ERROR; }
    len--;
    // Fall through to comma_delimited over the array contents.
    format = stream_format::comma_delimited;
  }
  return document_stream(*this, buf, len, batch_size, false, format);
}
inline simdjson_result<document_stream> parser::iterate_many(const char *buf, size_t len, size_t batch_size, stream_format format) noexcept {
  return iterate_many(reinterpret_cast<const uint8_t *>(buf), len, batch_size, format);
}
inline simdjson_result<document_stream> parser::iterate_many(padded_string_view s, size_t batch_size, stream_format format) noexcept {
  if (!s.has_sufficient_padding()) { return INSUFFICIENT_PADDING; }
  s.use_trailing_whitespace_as_padding();
  return iterate_many(s.data(), s.length(), batch_size, format);
}
inline simdjson_result<document_stream> parser::iterate_many(const std::string &s, size_t batch_size, stream_format format) noexcept {
  return iterate_many(padded_string_view(s), batch_size, format);
}
inline simdjson_result<document_stream> parser::iterate_many(std::string &s, size_t batch_size, stream_format format) noexcept {
  return iterate_many(pad(s), batch_size, format);
}
inline simdjson_result<document_stream> parser::iterate_many(const padded_string &s, size_t batch_size, stream_format format) noexcept {
  return iterate_many(padded_string_view(s), batch_size, format);
}
simdjson_pure simdjson_inline size_t parser::capacity() const noexcept {
  return _capacity;
}
simdjson_pure simdjson_inline size_t parser::max_capacity() const noexcept {
  return _max_capacity;
}
simdjson_pure simdjson_inline size_t parser::max_depth() const noexcept {
  return _max_depth;
}

simdjson_inline void parser::set_allocator(const buffer_allocator *allocator) noexcept {
  _allocator = allocator;
}
simdjson_pure simdjson_inline const buffer_allocator *parser::get_allocator() const noexcept {
  return _allocator;
}

simdjson_inline void parser::set_max_capacity(size_t max_capacity) noexcept {
  if(max_capacity > dom::MINIMAL_DOCUMENT_CAPACITY) {
    _max_capacity = max_capacity;
  } else {
    _max_capacity = dom::MINIMAL_DOCUMENT_CAPACITY;
  }
}

simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> parser::unescape(raw_json_string in, uint8_t *&dst, bool allow_replacement) const noexcept {
  uint8_t *end = implementation->parse_string(in.buf, dst, allow_replacement);
  if (!end) { return STRING_ERROR; }
  std::string_view result(reinterpret_cast<const char *>(dst), end-dst);
  dst = end;
  return result;
}

simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> parser::unescape_wobbly(raw_json_string in, uint8_t *&dst) const noexcept {
  uint8_t *end = implementation->parse_wobbly_string(in.buf, dst);
  if (!end) { return STRING_ERROR; }
  std::string_view result(reinterpret_cast<const char *>(dst), end-dst);
  dst = end;
  return result;
}

simdjson_inline simdjson_warn_unused ondemand::parser& parser::get_parser() {
  return *parser::get_parser_instance();
}

simdjson_inline bool parser::release_parser() {
  auto &parser_instance = parser::get_threadlocal_parser_if_exists();
  if (parser_instance) {
    parser_instance.reset();
    return true;
  }
  return false;
}

simdjson_inline simdjson_warn_unused std::unique_ptr<ondemand::parser>& parser::get_parser_instance() {
  std::unique_ptr<ondemand::parser>& parser_instance = get_threadlocal_parser_if_exists();
  if (!parser_instance) {
    parser_instance.reset(new ondemand::parser());
  }
  return parser_instance;
}

simdjson_inline simdjson_warn_unused std::unique_ptr<ondemand::parser>& parser::get_threadlocal_parser_if_exists() {
  // @the-moisrex points out that this could be implemented with std::optional (C++17).
  thread_local std::unique_ptr<ondemand::parser> parser_instance = nullptr;
  return parser_instance;
}


} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::parser>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::parser &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::parser>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::parser>(value)) {}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::parser>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::parser>(error) {}

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_PARSER_INL_H
