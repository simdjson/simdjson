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
  if (new_capacity > max_capacity()) { return CAPACITY; }
  if (string_buf && new_capacity == capacity() && new_max_depth == max_depth()) { return SUCCESS; }

  // string_capacity copied from document::allocate
  _capacity = 0;
  size_t string_capacity = SIMDJSON_ROUNDUP_N(5 * new_capacity / 3 + SIMDJSON_PADDING, 64);
  string_buf.reset(new (std::nothrow) uint8_t[string_capacity]);
#if SIMDJSON_DEVELOPMENT_CHECKS
  start_positions.reset(new (std::nothrow) token_position[new_max_depth]);
#endif
  if (implementation) {
    SIMDJSON_TRY( implementation->set_capacity(new_capacity) );
    SIMDJSON_TRY( implementation->set_max_depth(new_max_depth) );
  } else {
    SIMDJSON_TRY( simdjson::get_active_implementation()->create_dom_parser_implementation(new_capacity, new_max_depth, implementation) );
  }
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
  if (json.padding() < SIMDJSON_PADDING) { return INSUFFICIENT_PADDING; }

  json.remove_utf8_bom();

  // Allocate if needed
  if (capacity() < json.length() || !string_buf) {
    SIMDJSON_TRY( allocate(json.length(), max_depth()) );
  }

  // Run stage 1.
  SIMDJSON_TRY( implementation->stage1(reinterpret_cast<const uint8_t *>(json.data()), json.length(), stage1_mode::regular) );
  return document::start({ reinterpret_cast<const uint8_t *>(json.data()), this });
}

#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
simdjson_warn_unused simdjson_inline simdjson_result<document> parser::iterate_allow_incomplete_json(padded_string_view json) & noexcept {
  if (json.padding() < SIMDJSON_PADDING) { return INSUFFICIENT_PADDING; }

  json.remove_utf8_bom();

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
  return document::start({ reinterpret_cast<const uint8_t *>(json.data()), this, true });
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
  if(json.capacity() - json.size() < SIMDJSON_PADDING) {
    json.reserve(json.size() + SIMDJSON_PADDING);
  }
  return iterate(padded_string_view(json));
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
  if (json.padding() < SIMDJSON_PADDING) { return INSUFFICIENT_PADDING; }

  json.remove_utf8_bom();

  // Allocate if needed
  if (capacity() < json.length()) {
    SIMDJSON_TRY( allocate(json.length(), max_depth()) );
  }

  // Run stage 1.
  SIMDJSON_TRY( implementation->stage1(reinterpret_cast<const uint8_t *>(json.data()), json.length(), stage1_mode::regular) );
  return json_iterator(reinterpret_cast<const uint8_t *>(json.data()), this);
}

inline simdjson_result<document_stream> parser::iterate_many(const uint8_t *buf, size_t len, size_t batch_size, bool allow_comma_separated) noexcept {
  if(batch_size < MINIMAL_BATCH_SIZE) { batch_size = MINIMAL_BATCH_SIZE; }
  if((len >= 3) && (std::memcmp(buf, "\xEF\xBB\xBF", 3) == 0)) {
    buf += 3;
    len -= 3;
  }
  if(allow_comma_separated && batch_size < len) { batch_size = len; }
  return document_stream(*this, buf, len, batch_size, allow_comma_separated);
}
inline simdjson_result<document_stream> parser::iterate_many(const char *buf, size_t len, size_t batch_size, bool allow_comma_separated) noexcept {
  return iterate_many(reinterpret_cast<const uint8_t *>(buf), len, batch_size, allow_comma_separated);
}
inline simdjson_result<document_stream> parser::iterate_many(const std::string &s, size_t batch_size, bool allow_comma_separated) noexcept {
  return iterate_many(s.data(), s.length(), batch_size, allow_comma_separated);
}
inline simdjson_result<document_stream> parser::iterate_many(const padded_string &s, size_t batch_size, bool allow_comma_separated) noexcept {
  return iterate_many(s.data(), s.length(), batch_size, allow_comma_separated);
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

simdjson_inline void parser::set_max_capacity(size_t max_capacity) noexcept {
  if(max_capacity < dom::MINIMAL_DOCUMENT_CAPACITY) {
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