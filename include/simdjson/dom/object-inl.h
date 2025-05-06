#ifndef SIMDJSON_OBJECT_INL_H
#define SIMDJSON_OBJECT_INL_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/object.h"
#include "simdjson/dom/document.h"

#include "simdjson/dom/element-inl.h"
#include "simdjson/error-inl.h"
#include "simdjson/jsonpathutil.h"

#include <cstring>

namespace simdjson {

//
// simdjson_result<dom::object> inline implementation
//
simdjson_inline simdjson_result<dom::object>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::object>() {}
simdjson_inline simdjson_result<dom::object>::simdjson_result(dom::object value) noexcept
    : internal::simdjson_result_base<dom::object>(std::forward<dom::object>(value)) {}
simdjson_inline simdjson_result<dom::object>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<dom::object>(error) {}

inline simdjson_result<dom::element> simdjson_result<dom::object>::operator[](std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::operator[](const char *key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::at_pointer(std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at_pointer(json_pointer);
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::at_path(std::string_view json_path) const noexcept {
  auto json_pointer = json_path_to_pointer_conversion(json_path);
  if (json_pointer == "-1") { return INVALID_JSON_POINTER; }
  return at_pointer(json_pointer);
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::at_key(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::at_key_case_insensitive(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key_case_insensitive(key);
}

#if SIMDJSON_EXCEPTIONS

inline dom::object::iterator simdjson_result<dom::object>::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
inline dom::object::iterator simdjson_result<dom::object>::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}
inline size_t simdjson_result<dom::object>::size() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.size();
}

#endif // SIMDJSON_EXCEPTIONS

namespace dom {

//
// object inline implementation
//
simdjson_inline object::object() noexcept : tape{} {}
simdjson_inline object::object(const internal::tape_ref &_tape) noexcept : tape{_tape} { }
inline object::iterator object::begin() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  return internal::tape_ref(tape.doc, tape.json_index + 1);
}
inline object::iterator object::end() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  return internal::tape_ref(tape.doc, tape.after_element() - 1);
}
inline size_t object::size() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  return tape.scope_count();
}

inline simdjson_result<element> object::operator[](std::string_view key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> object::operator[](const char *key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> object::at_pointer(std::string_view json_pointer) const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  if(json_pointer.empty()) { // an empty string means that we return the current node
      return element(this->tape); // copy the current node
  } else if(json_pointer[0] != '/') { // otherwise there is an error
      return INVALID_JSON_POINTER;
  }
  json_pointer = json_pointer.substr(1);
  size_t slash = json_pointer.find('/');
  std::string_view key = json_pointer.substr(0, slash);
  // Grab the child with the given key
  simdjson_result<element> child;

  // If there is an escape character in the key, unescape it and then get the child.
  size_t escape = key.find('~');
  if (escape != std::string_view::npos) {
    // Unescape the key
    std::string unescaped(key);
    do {
      switch (unescaped[escape+1]) {
        case '0':
          unescaped.replace(escape, 2, "~");
          break;
        case '1':
          unescaped.replace(escape, 2, "/");
          break;
        default:
          return INVALID_JSON_POINTER; // "Unexpected ~ escape character in JSON pointer");
      }
      escape = unescaped.find('~', escape+1);
    } while (escape != std::string::npos);
    child = at_key(unescaped);
  } else {
    child = at_key(key);
  }
  if(child.error()) {
    return child; // we do not continue if there was an error
  }
  // If there is a /, we have to recurse and look up more of the path
  if (slash != std::string_view::npos) {
    child = child.at_pointer(json_pointer.substr(slash));
  }
  return child;
}

inline simdjson_result<element> object::at_path(std::string_view json_path) const noexcept {
  auto json_pointer = json_path_to_pointer_conversion(json_path);
  if (json_pointer == "-1") { return INVALID_JSON_POINTER; }
  return at_pointer(json_pointer);
}

inline simdjson_result<element> object::at_key(std::string_view key) const noexcept {
  iterator end_field = end();
  for (iterator field = begin(); field != end_field; ++field) {
    if (field.key_equals(key)) {
      return field.value();
    }
  }
  return NO_SUCH_FIELD;
}
// In case you wonder why we need this, please see
// https://github.com/simdjson/simdjson/issues/323
// People do seek keys in a case-insensitive manner.
inline simdjson_result<element> object::at_key_case_insensitive(std::string_view key) const noexcept {
  iterator end_field = end();
  for (iterator field = begin(); field != end_field; ++field) {
    if (field.key_equals_case_insensitive(key)) {
      return field.value();
    }
  }
  return NO_SUCH_FIELD;
}

inline object::operator element() const noexcept {
	return element(tape);
}

//
// object::iterator inline implementation
//
simdjson_inline object::iterator::iterator(const internal::tape_ref &_tape) noexcept : tape{_tape} { }
inline const key_value_pair object::iterator::operator*() const noexcept {
  return key_value_pair(key(), value());
}
inline bool object::iterator::operator!=(const object::iterator& other) const noexcept {
  return tape.json_index != other.tape.json_index;
}
inline bool object::iterator::operator==(const object::iterator& other) const noexcept {
  return tape.json_index == other.tape.json_index;
}
inline bool object::iterator::operator<(const object::iterator& other) const noexcept {
  return tape.json_index < other.tape.json_index;
}
inline bool object::iterator::operator<=(const object::iterator& other) const noexcept {
  return tape.json_index <= other.tape.json_index;
}
inline bool object::iterator::operator>=(const object::iterator& other) const noexcept {
  return tape.json_index >= other.tape.json_index;
}
inline bool object::iterator::operator>(const object::iterator& other) const noexcept {
  return tape.json_index > other.tape.json_index;
}
inline object::iterator& object::iterator::operator++() noexcept {
  tape.json_index++;
  tape.json_index = tape.after_element();
  return *this;
}
inline object::iterator object::iterator::operator++(int) noexcept {
  object::iterator out = *this;
  ++*this;
  return out;
}
inline std::string_view object::iterator::key() const noexcept {
  return tape.get_string_view();
}
inline uint32_t object::iterator::key_length() const noexcept {
  return tape.get_string_length();
}
inline const char* object::iterator::key_c_str() const noexcept {
  return reinterpret_cast<const char *>(&tape.doc->string_buf[size_t(tape.tape_value()) + sizeof(uint32_t)]);
}
inline element object::iterator::value() const noexcept {
  return element(internal::tape_ref(tape.doc, tape.json_index + 1));
}

/**
 * Design notes:
 * Instead of constructing a string_view and then comparing it with a
 * user-provided strings, it is probably more performant to have dedicated
 * functions taking as a parameter the string we want to compare against
 * and return true when they are equal. That avoids the creation of a temporary
 * std::string_view. Though it is possible for the compiler to avoid entirely
 * any overhead due to string_view, relying too much on compiler magic is
 * problematic: compiler magic sometimes fail, and then what do you do?
 * Also, enticing users to rely on high-performance function is probably better
 * on the long run.
 */

inline bool object::iterator::key_equals(std::string_view o) const noexcept {
  // We use the fact that the key length can be computed quickly
  // without access to the string buffer.
  const uint32_t len = key_length();
  if(o.size() == len) {
    // We avoid construction of a temporary string_view instance.
    return (memcmp(o.data(), key_c_str(), len) == 0);
  }
  return false;
}

inline bool object::iterator::key_equals_case_insensitive(std::string_view o) const noexcept {
  // We use the fact that the key length can be computed quickly
  // without access to the string buffer.
  const uint32_t len = key_length();
  if(o.size() == len) {
      // See For case-insensitive string comparisons, avoid char-by-char functions
      // https://lemire.me/blog/2020/04/30/for-case-insensitive-string-comparisons-avoid-char-by-char-functions/
      // Note that it might be worth rolling our own strncasecmp function, with vectorization.
      return (simdjson_strncasecmp(o.data(), key_c_str(), len) == 0);
  }
  return false;
}
//
// key_value_pair inline implementation
//
inline key_value_pair::key_value_pair(std::string_view _key, element _value) noexcept :
  key(_key), value(_value) {}

} // namespace dom

} // namespace simdjson

#if defined(__cpp_lib_ranges)
static_assert(std::ranges::view<simdjson::dom::object>);
static_assert(std::ranges::sized_range<simdjson::dom::object>);
#if SIMDJSON_EXCEPTIONS
static_assert(std::ranges::view<simdjson::simdjson_result<simdjson::dom::object>>);
static_assert(std::ranges::sized_range<simdjson::simdjson_result<simdjson::dom::object>>);
#endif // SIMDJSON_EXCEPTIONS
#endif // defined(__cpp_lib_ranges)

#endif // SIMDJSON_OBJECT_INL_H
