#ifndef SIMDJSON_ELEMENT_INL_H
#define SIMDJSON_ELEMENT_INL_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/element.h"
#include "simdjson/dom/document.h"
#include "simdjson/dom/object.h"
#include "simdjson/internal/tape_type.h"

#include "simdjson/dom/object-inl.h"
#include "simdjson/error-inl.h"

#include <ostream>
#include <limits>

namespace simdjson {

//
// simdjson_result<dom::element> inline implementation
//
simdjson_inline simdjson_result<dom::element>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::element>() {}
simdjson_inline simdjson_result<dom::element>::simdjson_result(dom::element &&value) noexcept
    : internal::simdjson_result_base<dom::element>(std::forward<dom::element>(value)) {}
simdjson_inline simdjson_result<dom::element>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<dom::element>(error) {}
inline simdjson_result<dom::element_type> simdjson_result<dom::element>::type() const noexcept {
  if (error()) { return error(); }
  return first.type();
}

template<typename T>
simdjson_inline bool simdjson_result<dom::element>::is() const noexcept {
  return !error() && first.is<T>();
}
template<typename T>
simdjson_inline simdjson_result<T> simdjson_result<dom::element>::get() const noexcept {
  if (error()) { return error(); }
  return first.get<T>();
}
template<typename T>
simdjson_warn_unused simdjson_inline error_code simdjson_result<dom::element>::get(T &value) const noexcept {
  if (error()) { return error(); }
  return first.get<T>(value);
}

simdjson_inline simdjson_result<dom::array> simdjson_result<dom::element>::get_array() const noexcept {
  if (error()) { return error(); }
  return first.get_array();
}
simdjson_inline simdjson_result<dom::object> simdjson_result<dom::element>::get_object() const noexcept {
  if (error()) { return error(); }
  return first.get_object();
}
simdjson_inline simdjson_result<const char *> simdjson_result<dom::element>::get_c_str() const noexcept {
  if (error()) { return error(); }
  return first.get_c_str();
}
simdjson_inline simdjson_result<size_t> simdjson_result<dom::element>::get_string_length() const noexcept {
  if (error()) { return error(); }
  return first.get_string_length();
}
simdjson_inline simdjson_result<std::string_view> simdjson_result<dom::element>::get_string() const noexcept {
  if (error()) { return error(); }
  return first.get_string();
}
simdjson_inline simdjson_result<int64_t> simdjson_result<dom::element>::get_int64() const noexcept {
  if (error()) { return error(); }
  return first.get_int64();
}
simdjson_inline simdjson_result<uint64_t> simdjson_result<dom::element>::get_uint64() const noexcept {
  if (error()) { return error(); }
  return first.get_uint64();
}
simdjson_inline simdjson_result<double> simdjson_result<dom::element>::get_double() const noexcept {
  if (error()) { return error(); }
  return first.get_double();
}
simdjson_inline simdjson_result<bool> simdjson_result<dom::element>::get_bool() const noexcept {
  if (error()) { return error(); }
  return first.get_bool();
}

simdjson_inline bool simdjson_result<dom::element>::is_array() const noexcept {
  return !error() && first.is_array();
}
simdjson_inline bool simdjson_result<dom::element>::is_object() const noexcept {
  return !error() && first.is_object();
}
simdjson_inline bool simdjson_result<dom::element>::is_string() const noexcept {
  return !error() && first.is_string();
}
simdjson_inline bool simdjson_result<dom::element>::is_int64() const noexcept {
  return !error() && first.is_int64();
}
simdjson_inline bool simdjson_result<dom::element>::is_uint64() const noexcept {
  return !error() && first.is_uint64();
}
simdjson_inline bool simdjson_result<dom::element>::is_double() const noexcept {
  return !error() && first.is_double();
}
simdjson_inline bool simdjson_result<dom::element>::is_number() const noexcept {
  return !error() && first.is_number();
}
simdjson_inline bool simdjson_result<dom::element>::is_bool() const noexcept {
  return !error() && first.is_bool();
}

simdjson_inline bool simdjson_result<dom::element>::is_null() const noexcept {
  return !error() && first.is_null();
}

simdjson_inline simdjson_result<dom::element> simdjson_result<dom::element>::operator[](std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_inline simdjson_result<dom::element> simdjson_result<dom::element>::operator[](const char *key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_inline simdjson_result<dom::element> simdjson_result<dom::element>::at_pointer(const std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at_pointer(json_pointer);
}
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
[[deprecated("For standard compliance, use at_pointer instead, and prefix your pointers with a slash '/', see RFC6901 ")]]
simdjson_inline simdjson_result<dom::element> simdjson_result<dom::element>::at(const std::string_view json_pointer) const noexcept {
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
  if (error()) { return error(); }
  return first.at(json_pointer);
SIMDJSON_POP_DISABLE_WARNINGS
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API
simdjson_inline simdjson_result<dom::element> simdjson_result<dom::element>::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}
simdjson_inline simdjson_result<dom::element> simdjson_result<dom::element>::at_key(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}
simdjson_inline simdjson_result<dom::element> simdjson_result<dom::element>::at_key_case_insensitive(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key_case_insensitive(key);
}

#if SIMDJSON_EXCEPTIONS

simdjson_inline simdjson_result<dom::element>::operator bool() const noexcept(false) {
  return get<bool>();
}
simdjson_inline simdjson_result<dom::element>::operator const char *() const noexcept(false) {
  return get<const char *>();
}
simdjson_inline simdjson_result<dom::element>::operator std::string_view() const noexcept(false) {
  return get<std::string_view>();
}
simdjson_inline simdjson_result<dom::element>::operator uint64_t() const noexcept(false) {
  return get<uint64_t>();
}
simdjson_inline simdjson_result<dom::element>::operator int64_t() const noexcept(false) {
  return get<int64_t>();
}
simdjson_inline simdjson_result<dom::element>::operator double() const noexcept(false) {
  return get<double>();
}
simdjson_inline simdjson_result<dom::element>::operator dom::array() const noexcept(false) {
  return get<dom::array>();
}
simdjson_inline simdjson_result<dom::element>::operator dom::object() const noexcept(false) {
  return get<dom::object>();
}

simdjson_inline dom::array::iterator simdjson_result<dom::element>::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
simdjson_inline dom::array::iterator simdjson_result<dom::element>::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

namespace dom {

//
// element inline implementation
//
simdjson_inline element::element() noexcept : tape{} {}
simdjson_inline element::element(const internal::tape_ref &_tape) noexcept : tape{_tape} { }

inline element_type element::type() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  auto tape_type = tape.tape_ref_type();
  return tape_type == internal::tape_type::FALSE_VALUE ? element_type::BOOL : static_cast<element_type>(tape_type);
}

inline simdjson_result<bool> element::get_bool() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  if(tape.is_true()) {
    return true;
  } else if(tape.is_false()) {
    return false;
  }
  return INCORRECT_TYPE;
}
inline simdjson_result<const char *> element::get_c_str() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  switch (tape.tape_ref_type()) {
    case internal::tape_type::STRING: {
      return tape.get_c_str();
    }
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<size_t> element::get_string_length() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  switch (tape.tape_ref_type()) {
    case internal::tape_type::STRING: {
      return tape.get_string_length();
    }
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<std::string_view> element::get_string() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  switch (tape.tape_ref_type()) {
    case internal::tape_type::STRING:
      return tape.get_string_view();
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<uint64_t> element::get_uint64() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  if(simdjson_unlikely(!tape.is_uint64())) { // branch rarely taken
    if(tape.is_int64()) {
      int64_t result = tape.next_tape_value<int64_t>();
      if (result < 0) {
        return NUMBER_OUT_OF_RANGE;
      }
      return uint64_t(result);
    }
    return INCORRECT_TYPE;
  }
  return tape.next_tape_value<int64_t>();
}
inline simdjson_result<int64_t> element::get_int64() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  if(simdjson_unlikely(!tape.is_int64())) { // branch rarely taken
    if(tape.is_uint64()) {
      uint64_t result = tape.next_tape_value<uint64_t>();
      // Wrapping max in parens to handle Windows issue: https://stackoverflow.com/questions/11544073/how-do-i-deal-with-the-max-macro-in-windows-h-colliding-with-max-in-std
      if (result > uint64_t((std::numeric_limits<int64_t>::max)())) {
        return NUMBER_OUT_OF_RANGE;
      }
      return static_cast<int64_t>(result);
    }
    return INCORRECT_TYPE;
  }
  return tape.next_tape_value<int64_t>();
}
inline simdjson_result<double> element::get_double() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  // Performance considerations:
  // 1. Querying tape_ref_type() implies doing a shift, it is fast to just do a straight
  //   comparison.
  // 2. Using a switch-case relies on the compiler guessing what kind of code generation
  //    we want... But the compiler cannot know that we expect the type to be "double"
  //    most of the time.
  // We can expect get<double> to refer to a double type almost all the time.
  // It is important to craft the code accordingly so that the compiler can use this
  // information. (This could also be solved with profile-guided optimization.)
  if(simdjson_unlikely(!tape.is_double())) { // branch rarely taken
    if(tape.is_uint64()) {
      return double(tape.next_tape_value<uint64_t>());
    } else if(tape.is_int64()) {
      return double(tape.next_tape_value<int64_t>());
    }
    return INCORRECT_TYPE;
  }
  // this is common:
  return tape.next_tape_value<double>();
}
inline simdjson_result<array> element::get_array() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  switch (tape.tape_ref_type()) {
    case internal::tape_type::START_ARRAY:
      return array(tape);
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<object> element::get_object() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  switch (tape.tape_ref_type()) {
    case internal::tape_type::START_OBJECT:
      return object(tape);
    default:
      return INCORRECT_TYPE;
  }
}

template<typename T>
simdjson_warn_unused simdjson_inline error_code element::get(T &value) const noexcept {
  return get<T>().get(value);
}
// An element-specific version prevents recursion with simdjson_result::get<element>(value)
template<>
simdjson_warn_unused simdjson_inline error_code element::get<element>(element &value) const noexcept {
  value = element(tape);
  return SUCCESS;
}
template<typename T>
inline void element::tie(T &value, error_code &error) && noexcept {
  error = get<T>(value);
}

template<typename T>
simdjson_inline bool element::is() const noexcept {
  auto result = get<T>();
  return !result.error();
}

template<> inline simdjson_result<array> element::get<array>() const noexcept { return get_array(); }
template<> inline simdjson_result<object> element::get<object>() const noexcept { return get_object(); }
template<> inline simdjson_result<const char *> element::get<const char *>() const noexcept { return get_c_str(); }
template<> inline simdjson_result<std::string_view> element::get<std::string_view>() const noexcept { return get_string(); }
template<> inline simdjson_result<int64_t> element::get<int64_t>() const noexcept { return get_int64(); }
template<> inline simdjson_result<uint64_t> element::get<uint64_t>() const noexcept { return get_uint64(); }
template<> inline simdjson_result<double> element::get<double>() const noexcept { return get_double(); }
template<> inline simdjson_result<bool> element::get<bool>() const noexcept { return get_bool(); }

inline bool element::is_array() const noexcept { return is<array>(); }
inline bool element::is_object() const noexcept { return is<object>(); }
inline bool element::is_string() const noexcept { return is<std::string_view>(); }
inline bool element::is_int64() const noexcept { return is<int64_t>(); }
inline bool element::is_uint64() const noexcept { return is<uint64_t>(); }
inline bool element::is_double() const noexcept { return is<double>(); }
inline bool element::is_bool() const noexcept { return is<bool>(); }
inline bool element::is_number() const noexcept { return is_int64() || is_uint64() || is_double(); }

inline bool element::is_null() const noexcept {
  return tape.is_null_on_tape();
}

#if SIMDJSON_EXCEPTIONS

inline element::operator bool() const noexcept(false) { return get<bool>(); }
inline element::operator const char*() const noexcept(false) { return get<const char *>(); }
inline element::operator std::string_view() const noexcept(false) { return get<std::string_view>(); }
inline element::operator uint64_t() const noexcept(false) { return get<uint64_t>(); }
inline element::operator int64_t() const noexcept(false) { return get<int64_t>(); }
inline element::operator double() const noexcept(false) { return get<double>(); }
inline element::operator array() const noexcept(false) { return get<array>(); }
inline element::operator object() const noexcept(false) { return get<object>(); }

inline array::iterator element::begin() const noexcept(false) {
  return get<array>().begin();
}
inline array::iterator element::end() const noexcept(false) {
  return get<array>().end();
}

#endif // SIMDJSON_EXCEPTIONS

inline simdjson_result<element> element::operator[](std::string_view key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> element::operator[](const char *key) const noexcept {
  return at_key(key);
}

inline simdjson_result<element> element::at_pointer(std::string_view json_pointer) const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  switch (tape.tape_ref_type()) {
    case internal::tape_type::START_OBJECT:
      return object(tape).at_pointer(json_pointer);
    case internal::tape_type::START_ARRAY:
      return array(tape).at_pointer(json_pointer);
    default: {
      if(!json_pointer.empty()) { // a non-empty string is invalid on an atom
        return INVALID_JSON_POINTER;
      }
      // an empty string means that we return the current node
      dom::element copy(*this);
      return simdjson_result<element>(std::move(copy));
    }
  }
}
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
[[deprecated("For standard compliance, use at_pointer instead, and prefix your pointers with a slash '/', see RFC6901 ")]]
inline simdjson_result<element> element::at(std::string_view json_pointer) const noexcept {
  // version 0.4 of simdjson allowed non-compliant pointers
  auto std_pointer = (json_pointer.empty() ? "" : "/") + std::string(json_pointer.begin(), json_pointer.end());
  return at_pointer(std_pointer);
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API

inline simdjson_result<element> element::at(size_t index) const noexcept {
  return get<array>().at(index);
}
inline simdjson_result<element> element::at_key(std::string_view key) const noexcept {
  return get<object>().at_key(key);
}
inline simdjson_result<element> element::at_key_case_insensitive(std::string_view key) const noexcept {
  return get<object>().at_key_case_insensitive(key);
}
inline bool element::operator<(const element &other) const noexcept {
  return tape.json_index < other.tape.json_index;
}
inline bool element::operator==(const element &other) const noexcept {
  return tape.json_index == other.tape.json_index;
}

inline bool element::dump_raw_tape(std::ostream &out) const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  return tape.doc->dump_raw_tape(out);
}


inline std::ostream& operator<<(std::ostream& out, element_type type) {
  switch (type) {
    case element_type::ARRAY:
      return out << "array";
    case element_type::OBJECT:
      return out << "object";
    case element_type::INT64:
      return out << "int64_t";
    case element_type::UINT64:
      return out << "uint64_t";
    case element_type::DOUBLE:
      return out << "double";
    case element_type::STRING:
      return out << "string";
    case element_type::BOOL:
      return out << "bool";
    case element_type::NULL_VALUE:
      return out << "null";
    default:
      return out << "unexpected content!!!"; // abort() usage is forbidden in the library
  }
}

} // namespace dom

} // namespace simdjson

#endif // SIMDJSON_ELEMENT_INL_H
