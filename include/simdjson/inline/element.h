#ifndef SIMDJSON_INLINE_ELEMENT_H
#define SIMDJSON_INLINE_ELEMENT_H

#include "simdjson/dom/array.h"
#include "simdjson/dom/element.h"
#include "simdjson/dom/object.h"
#include <cstring>
#include <utility>

namespace simdjson {

//
// simdjson_result<dom::element> inline implementation
//
really_inline simdjson_result<dom::element>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::element>() {}
really_inline simdjson_result<dom::element>::simdjson_result(dom::element &&value) noexcept
    : internal::simdjson_result_base<dom::element>(std::forward<dom::element>(value)) {}
really_inline simdjson_result<dom::element>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<dom::element>(error) {}
inline simdjson_result<dom::element_type> simdjson_result<dom::element>::type() const noexcept {
  if (error()) { return error(); }
  return first.type();
}
inline simdjson_result<bool> simdjson_result<dom::element>::is_null() const noexcept {
  if (error()) { return error(); }
  return first.is_null();
}
template<typename T>
inline simdjson_result<bool> simdjson_result<dom::element>::is() const noexcept {
  if (error()) { return error(); }
  return first.is<T>();
}
template<typename T>
inline simdjson_result<T> simdjson_result<dom::element>::get() const noexcept {
  if (error()) { return error(); }
  return first.get<T>();
}

inline simdjson_result<dom::element> simdjson_result<dom::element>::operator[](const std::string_view &key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline simdjson_result<dom::element> simdjson_result<dom::element>::operator[](const char *key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline simdjson_result<dom::element> simdjson_result<dom::element>::at(const std::string_view &json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at(json_pointer);
}
inline simdjson_result<dom::element> simdjson_result<dom::element>::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}
inline simdjson_result<dom::element> simdjson_result<dom::element>::at_key(const std::string_view &key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}
inline simdjson_result<dom::element> simdjson_result<dom::element>::at_key_case_insensitive(const std::string_view &key) const noexcept {
  if (error()) { return error(); }
  return first.at_key_case_insensitive(key);
}

#if SIMDJSON_EXCEPTIONS

inline simdjson_result<dom::element>::operator bool() const noexcept(false) {
  return get<bool>();
}
inline simdjson_result<dom::element>::operator const char *() const noexcept(false) {
  return get<const char *>();
}
inline simdjson_result<dom::element>::operator std::string_view() const noexcept(false) {
  return get<std::string_view>();
}
inline simdjson_result<dom::element>::operator uint64_t() const noexcept(false) {
  return get<uint64_t>();
}
inline simdjson_result<dom::element>::operator int64_t() const noexcept(false) {
  return get<int64_t>();
}
inline simdjson_result<dom::element>::operator double() const noexcept(false) {
  return get<double>();
}
inline simdjson_result<dom::element>::operator dom::array() const noexcept(false) {
  return get<dom::array>();
}
inline simdjson_result<dom::element>::operator dom::object() const noexcept(false) {
  return get<dom::object>();
}

inline dom::array::iterator simdjson_result<dom::element>::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
inline dom::array::iterator simdjson_result<dom::element>::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif

namespace dom {

//
// element inline implementation
//
really_inline element::element() noexcept : internal::tape_ref() {}
really_inline element::element(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) { }

inline element_type element::type() const noexcept {
  auto tape_type = tape_ref_type();
  return tape_type == internal::tape_type::FALSE_VALUE ? element_type::BOOL : static_cast<element_type>(tape_type);
}
really_inline bool element::is_null() const noexcept {
  return is_null_on_tape();
}

template<>
inline simdjson_result<bool> element::get<bool>() const noexcept {
  if(is_true()) {
    return true;
  } else if(is_false()) {
    return false;
  }
  return INCORRECT_TYPE;
}
template<>
inline simdjson_result<const char *> element::get<const char *>() const noexcept {
  switch (tape_ref_type()) {
    case internal::tape_type::STRING: {
      return get_c_str();
    }
    default:
      return INCORRECT_TYPE;
  }
}
template<>
inline simdjson_result<std::string_view> element::get<std::string_view>() const noexcept {
  switch (tape_ref_type()) {
    case internal::tape_type::STRING:
      return get_string_view();
    default:
      return INCORRECT_TYPE;
  }
}
template<>
inline simdjson_result<uint64_t> element::get<uint64_t>() const noexcept {
  if(unlikely(!is_uint64())) { // branch rarely taken
    if(is_int64()) {
      int64_t result = next_tape_value<int64_t>();
      if (result < 0) {
        return NUMBER_OUT_OF_RANGE;
      }
      return uint64_t(result);
    }
    return INCORRECT_TYPE;
  }
  return next_tape_value<int64_t>();
}
template<>
inline simdjson_result<int64_t> element::get<int64_t>() const noexcept {
  if(unlikely(!is_int64())) { // branch rarely taken
    if(is_uint64()) {
      uint64_t result = next_tape_value<uint64_t>();
      // Wrapping max in parens to handle Windows issue: https://stackoverflow.com/questions/11544073/how-do-i-deal-with-the-max-macro-in-windows-h-colliding-with-max-in-std
      if (result > uint64_t((std::numeric_limits<int64_t>::max)())) {
        return NUMBER_OUT_OF_RANGE;
      }
      return static_cast<int64_t>(result);
    }
    return INCORRECT_TYPE;
  }
  return next_tape_value<int64_t>();
}
template<>
inline simdjson_result<double> element::get<double>() const noexcept {
  // Performance considerations:
  // 1. Querying tape_ref_type() implies doing a shift, it is fast to just do a straight
  //   comparison.
  // 2. Using a switch-case relies on the compiler guessing what kind of code generation
  //    we want... But the compiler cannot know that we expect the type to be "double"
  //    most of the time.
  // We can expect get<double> to refer to a double type almost all the time.
  // It is important to craft the code accordingly so that the compiler can use this
  // information. (This could also be solved with profile-guided optimization.)
  if(unlikely(!is_double())) { // branch rarely taken
    if(is_uint64()) {
      return double(next_tape_value<uint64_t>());
    } else if(is_int64()) {
      return double(next_tape_value<int64_t>());
    }
    return INCORRECT_TYPE;
  }
  // this is common:
  return next_tape_value<double>();
}
template<>
inline simdjson_result<array> element::get<array>() const noexcept {
  switch (tape_ref_type()) {
    case internal::tape_type::START_ARRAY:
      return array(doc, json_index);
    default:
      return INCORRECT_TYPE;
  }
}
template<>
inline simdjson_result<object> element::get<object>() const noexcept {
  switch (tape_ref_type()) {
    case internal::tape_type::START_OBJECT:
      return object(doc, json_index);
    default:
      return INCORRECT_TYPE;
  }
}

template<typename T>
really_inline bool element::is() const noexcept {
  auto result = get<T>();
  return !result.error();
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

inline simdjson_result<element> element::operator[](const std::string_view &key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> element::operator[](const char *key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> element::at(const std::string_view &json_pointer) const noexcept {
  switch (tape_ref_type()) {
    case internal::tape_type::START_OBJECT:
      return object(doc, json_index).at(json_pointer);
    case internal::tape_type::START_ARRAY:
      return array(doc, json_index).at(json_pointer);
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<element> element::at(size_t index) const noexcept {
  return get<array>().at(index);
}
inline simdjson_result<element> element::at_key(const std::string_view &key) const noexcept {
  return get<object>().at_key(key);
}
inline simdjson_result<element> element::at_key_case_insensitive(const std::string_view &key) const noexcept {
  return get<object>().at_key_case_insensitive(key);
}

inline bool element::dump_raw_tape(std::ostream &out) const noexcept {
  return doc->dump_raw_tape(out);
}

inline std::ostream& operator<<(std::ostream& out, const element &value) {
  return out << minify<element>(value);
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
      abort();
  }
}

} // namespace dom

template<>
inline std::ostream& minify<dom::element>::print(std::ostream& out) {
  using tape_type=internal::tape_type;
  size_t depth = 0;
  constexpr size_t MAX_DEPTH = 16;
  bool is_object[MAX_DEPTH];
  is_object[0] = false;
  bool after_value = false;

  internal::tape_ref iter(value);
  do {
    // print commas after each value
    if (after_value) {
      out << ",";
    }
    // If we are in an object, print the next key and :, and skip to the next value.
    if (is_object[depth]) {
      out << '"' << internal::escape_json_string(iter.get_string_view()) << "\":";
      iter.json_index++;
    }
    switch (iter.tape_ref_type()) {

    // Arrays
    case tape_type::START_ARRAY: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (unlikely(depth >= MAX_DEPTH)) {
        out << minify<dom::array>(dom::array(iter.doc, iter.json_index));
        iter.json_index = iter.matching_brace_index() - 1; // Jump to the ]
        depth--;
        break;
      }

      // Output start [
      out << '[';
      iter.json_index++;

      // Handle empty [] (we don't want to come back around and print commas)
      if (iter.tape_ref_type() == tape_type::END_ARRAY) {
        out << ']';
        depth--;
        break;
      }

      is_object[depth] = false;
      after_value = false;
      continue;
    }

    // Objects
    case tape_type::START_OBJECT: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (unlikely(depth >= MAX_DEPTH)) {
        out << minify<dom::object>(dom::object(iter.doc, iter.json_index));
        iter.json_index = iter.matching_brace_index() - 1; // Jump to the }
        depth--;
        break;
      }

      // Output start {
      out << '{';
      iter.json_index++;

      // Handle empty {} (we don't want to come back around and print commas)
      if (iter.tape_ref_type() == tape_type::END_OBJECT) {
        out << '}';
        depth--;
        break;
      }

      is_object[depth] = true;
      after_value = false;
      continue;
    }

    // Scalars
    case tape_type::STRING:
      out << '"' << internal::escape_json_string(iter.get_string_view()) << '"';
      break;
    case tape_type::INT64:
      out << iter.next_tape_value<int64_t>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::UINT64:
      out << iter.next_tape_value<uint64_t>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::DOUBLE:
      out << iter.next_tape_value<double>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::TRUE_VALUE:
      out << "true";
      break;
    case tape_type::FALSE_VALUE:
      out << "false";
      break;
    case tape_type::NULL_VALUE:
      out << "null";
      break;

    // These are impossible
    case tape_type::END_ARRAY:
    case tape_type::END_OBJECT:
    case tape_type::ROOT:
      abort();
    }
    iter.json_index++;
    after_value = true;

    // Handle multiple ends in a row
    while (depth != 0 && (iter.tape_ref_type() == tape_type::END_ARRAY || iter.tape_ref_type() == tape_type::END_OBJECT)) {
      out << char(iter.tape_ref_type());
      depth--;
      iter.json_index++;
    }

    // Stop when we're at depth 0
  } while (depth != 0);

  return out;
}

#if SIMDJSON_EXCEPTIONS

template<>
inline std::ostream& minify<simdjson_result<dom::element>>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<dom::element>(value.first);
}

inline std::ostream& operator<<(std::ostream& out, const simdjson_result<dom::element> &value) noexcept(false) {
  return out << minify<simdjson_result<dom::element>>(value);
}
#endif

} // namespace simdjson

#endif // SIMDJSON_INLINE_ELEMENT_H