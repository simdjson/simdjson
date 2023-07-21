#ifndef SIMDJSON_GENERIC_ONDEMAND_RAW_JSON_STRING_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_RAW_JSON_STRING_INL_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/raw_json_string.h"
#include "simdjson/generic/ondemand/json_iterator-inl.h"
#include "simdjson/generic/implementation_simdjson_result_base-inl.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {

namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_inline raw_json_string::raw_json_string(const uint8_t * _buf) noexcept : buf{_buf} {}

simdjson_inline const char * raw_json_string::raw() const noexcept { return reinterpret_cast<const char *>(buf); }


simdjson_inline bool raw_json_string::is_free_from_unescaped_quote(std::string_view target) noexcept {
  size_t pos{0};
  // if the content has no escape character, just scan through it quickly!
  for(;pos < target.size() && target[pos] != '\\';pos++) {}
  // slow path may begin.
  bool escaping{false};
  for(;pos < target.size();pos++) {
    if((target[pos] == '"') && !escaping) {
      return false;
    } else if(target[pos] == '\\') {
      escaping = !escaping;
    } else {
      escaping = false;
    }
  }
  return true;
}

simdjson_inline bool raw_json_string::is_free_from_unescaped_quote(const char* target) noexcept {
  size_t pos{0};
  // if the content has no escape character, just scan through it quickly!
  for(;target[pos] && target[pos] != '\\';pos++) {}
  // slow path may begin.
  bool escaping{false};
  for(;target[pos];pos++) {
    if((target[pos] == '"') && !escaping) {
      return false;
    } else if(target[pos] == '\\') {
      escaping = !escaping;
    } else {
      escaping = false;
    }
  }
  return true;
}


simdjson_inline bool raw_json_string::unsafe_is_equal(size_t length, std::string_view target) const noexcept {
  // If we are going to call memcmp, then we must know something about the length of the raw_json_string.
  return (length >= target.size()) && (raw()[target.size()] == '"') && !memcmp(raw(), target.data(), target.size());
}

simdjson_inline bool raw_json_string::unsafe_is_equal(std::string_view target) const noexcept {
  // Assumptions: does not contain unescaped quote characters, and
  // the raw content is quote terminated within a valid JSON string.
  if(target.size() <= SIMDJSON_PADDING) {
    return (raw()[target.size()] == '"') && !memcmp(raw(), target.data(), target.size());
  }
  const char * r{raw()};
  size_t pos{0};
  for(;pos < target.size();pos++) {
    if(r[pos] != target[pos]) { return false; }
  }
  if(r[pos] != '"') { return false; }
  return true;
}

simdjson_inline bool raw_json_string::is_equal(std::string_view target) const noexcept {
  const char * r{raw()};
  size_t pos{0};
  bool escaping{false};
  for(;pos < target.size();pos++) {
    if(r[pos] != target[pos]) { return false; }
    // if target is a compile-time constant and it is free from
    // quotes, then the next part could get optimized away through
    // inlining.
    if((target[pos] == '"') && !escaping) {
      // We have reached the end of the raw_json_string but
      // the target is not done.
      return false;
    } else if(target[pos] == '\\') {
      escaping = !escaping;
    } else {
      escaping = false;
    }
  }
  if(r[pos] != '"') { return false; }
  return true;
}


simdjson_inline bool raw_json_string::unsafe_is_equal(const char * target) const noexcept {
  // Assumptions: 'target' does not contain unescaped quote characters, is null terminated and
  // the raw content is quote terminated within a valid JSON string.
  const char * r{raw()};
  size_t pos{0};
  for(;target[pos];pos++) {
    if(r[pos] != target[pos]) { return false; }
  }
  if(r[pos] != '"') { return false; }
  return true;
}

simdjson_inline bool raw_json_string::is_equal(const char* target) const noexcept {
  // Assumptions: does not contain unescaped quote characters, and
  // the raw content is quote terminated within a valid JSON string.
  const char * r{raw()};
  size_t pos{0};
  bool escaping{false};
  for(;target[pos];pos++) {
    if(r[pos] != target[pos]) { return false; }
    // if target is a compile-time constant and it is free from
    // quotes, then the next part could get optimized away through
    // inlining.
    if((target[pos] == '"') && !escaping) {
      // We have reached the end of the raw_json_string but
      // the target is not done.
      return false;
    } else if(target[pos] == '\\') {
      escaping = !escaping;
    } else {
      escaping = false;
    }
  }
  if(r[pos] != '"') { return false; }
  return true;
}

simdjson_unused simdjson_inline bool operator==(const raw_json_string &a, std::string_view c) noexcept {
  return a.unsafe_is_equal(c);
}

simdjson_unused simdjson_inline bool operator==(std::string_view c, const raw_json_string &a) noexcept {
  return a == c;
}

simdjson_unused simdjson_inline bool operator!=(const raw_json_string &a, std::string_view c) noexcept {
  return !(a == c);
}

simdjson_unused simdjson_inline bool operator!=(std::string_view c, const raw_json_string &a) noexcept {
  return !(a == c);
}


simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> raw_json_string::unescape(json_iterator &iter, bool allow_replacement) const noexcept {
  return iter.unescape(*this, allow_replacement);
}

simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> raw_json_string::unescape_wobbly(json_iterator &iter) const noexcept {
  return iter.unescape_wobbly(*this);
}

simdjson_unused simdjson_inline std::ostream &operator<<(std::ostream &out, const raw_json_string &str) noexcept {
  bool in_escape = false;
  const char *s = str.raw();
  while (true) {
    switch (*s) {
      case '\\': in_escape = !in_escape; break;
      case '"': if (in_escape) { in_escape = false; } else { return out; } break;
      default: if (in_escape) { in_escape = false; }
    }
    out << *s;
    s++;
  }
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>(value)) {}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>(error) {}

simdjson_inline simdjson_result<const char *> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>::raw() const noexcept {
  if (error()) { return error(); }
  return first.raw();
}
simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>::unescape(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &iter, bool allow_replacement) const noexcept {
  if (error()) { return error(); }
  return first.unescape(iter, allow_replacement);
}
simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>::unescape_wobbly(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &iter) const noexcept {
  if (error()) { return error(); }
  return first.unescape_wobbly(iter);
}
} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_RAW_JSON_STRING_INL_H