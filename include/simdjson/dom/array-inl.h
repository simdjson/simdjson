#ifndef SIMDJSON_ARRAY_INL_H
#define SIMDJSON_ARRAY_INL_H

#include <utility>

#include "simdjson/dom/base.h"
#include "simdjson/dom/array.h"
#include "simdjson/dom/element.h"
#include "simdjson/error-inl.h"
#include "simdjson/jsonpathutil.h"
#include "simdjson/internal/tape_ref-inl.h"

#include <limits>

namespace simdjson {

//
// simdjson_result<dom::array> inline implementation
//
simdjson_inline simdjson_result<dom::array>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::array>() {}
simdjson_inline simdjson_result<dom::array>::simdjson_result(dom::array value) noexcept
    : internal::simdjson_result_base<dom::array>(std::forward<dom::array>(value)) {}
simdjson_inline simdjson_result<dom::array>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<dom::array>(error) {}

#if SIMDJSON_EXCEPTIONS

inline dom::array::iterator simdjson_result<dom::array>::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
inline dom::array::iterator simdjson_result<dom::array>::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}
inline size_t simdjson_result<dom::array>::size() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.size();
}

#endif // SIMDJSON_EXCEPTIONS

inline simdjson_result<dom::element> simdjson_result<dom::array>::at_pointer(std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at_pointer(json_pointer);
}

 inline simdjson_result<dom::element> simdjson_result<dom::array>::at_path(std::string_view json_path) const noexcept {
  auto json_pointer = json_path_to_pointer_conversion(json_path);
  if (json_pointer == "-1") { return INVALID_JSON_POINTER; }
  return at_pointer(json_pointer);
 }

inline simdjson_result<std::vector<dom::element>> simdjson_result<dom::array>::at_path_with_wildcard(std::string_view json_path) const noexcept {
  if (error()) {
    return error();
  }
  return first.at_path_with_wildcard(json_path);
}

inline simdjson_result<dom::element> simdjson_result<dom::array>::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}

inline std::vector<dom::element>& simdjson_result<dom::array>::get_values(std::vector<dom::element>& out) const noexcept {
  return first.get_values(out);
}

namespace dom {

//
// array inline implementation
//
simdjson_inline array::array() noexcept : tape{} {}
simdjson_inline array::array(const internal::tape_ref &_tape) noexcept : tape{_tape} {}
inline array::iterator array::begin() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  return internal::tape_ref(tape.doc, tape.json_index + 1);
}
inline array::iterator array::end() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  return internal::tape_ref(tape.doc, tape.after_element() - 1);
}
inline size_t array::size() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  return tape.scope_count();
}
inline size_t array::number_of_slots() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  return tape.matching_brace_index() - tape.json_index;
}
inline simdjson_result<element> array::at_pointer(std::string_view json_pointer) const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  if(json_pointer.empty()) { // an empty string means that we return the current node
      return element(this->tape); // copy the current node
  } else if(json_pointer[0] != '/') { // otherwise there is an error
      return INVALID_JSON_POINTER;
  }
  json_pointer = json_pointer.substr(1);
  // - means "the append position" or "the element after the end of the array"
  // We don't support this, because we're returning a real element, not a position.
  if (json_pointer == "-") { return INDEX_OUT_OF_BOUNDS; }

  // Read the array index
  size_t array_index = 0;
  size_t i;
  for (i = 0; i < json_pointer.length() && json_pointer[i] != '/'; i++) {
    uint8_t digit = uint8_t(json_pointer[i] - '0');
    // Check for non-digit in array index. If it's there, we're trying to get a field in an object
    if (digit > 9) { return INCORRECT_TYPE; }
    array_index = array_index*10 + digit;
  }

  // 0 followed by other digits is invalid
  if (i > 1 && json_pointer[0] == '0') { return INVALID_JSON_POINTER; } // "JSON pointer array index has other characters after 0"

  // Empty string is invalid; so is a "/" with no digits before it
  if (i == 0) { return INVALID_JSON_POINTER; } // "Empty string in JSON pointer array index"

  // Get the child
  auto child = array(tape).at(array_index);
  // If there is an error, it ends here
  if(child.error()) {
    return child;
  }
  // If there is a /, we're not done yet, call recursively.
  if (i < json_pointer.length()) {
    child = child.at_pointer(json_pointer.substr(i));
  }
  return child;
}

inline simdjson_result<element> array::at_path(std::string_view json_path) const noexcept {
  auto json_pointer = json_path_to_pointer_conversion(json_path);
  if (json_pointer == "-1") { return INVALID_JSON_POINTER; }
  return at_pointer(json_pointer);
}

inline void array::process_json_path_of_child_elements(std::vector<element>::iterator& current, std::vector<element>::iterator& end, const std::string_view& path_suffix, std::vector<element>& accumulator) const noexcept {
  if (current == end) {
    return;
  }

  simdjson_result<std::vector<element>> result;


  for (auto it = current; it != end; ++it) {
    std::vector<element> child_result;
    auto error = it->at_path_with_wildcard(path_suffix).get(child_result);
    if(error) {
      continue;
    }
    accumulator.reserve(accumulator.size() + child_result.size());
    accumulator.insert(accumulator.end(),
                        std::make_move_iterator(child_result.begin()),
                        std::make_move_iterator(child_result.end()));
  }
}

inline simdjson_result<std::vector<element>> array::at_path_with_wildcard(std::string_view json_path) const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914

  size_t i = 0;
   // json_path.starts_with('$') requires C++20.
  if (!json_path.empty() && json_path.front() == '$') {
    i = 1;
  }

  if (i >= json_path.size() || (json_path[i] != '.' && json_path[i] != '[')) {
    return INVALID_JSON_POINTER;
  }

  if (json_path.find("*") != std::string::npos) {
    std::vector<element> child_values;

    if (
      (json_path.compare(i, 3, "[*]") == 0 && json_path.size() == i + 3) ||
      (json_path.compare(i, 2,".*") == 0 && json_path.size() == i + 2)
    ) {
      get_values(child_values);
      return child_values;
    }

    std::pair<std::string_view, std::string_view> key_and_json_path = get_next_key_and_json_path(json_path);

    std::string_view key = key_and_json_path.first;
    json_path = key_and_json_path.second;

    if (key.size() > 0) {
      if (key == "*") {
        get_values(child_values);
      } else {
        element pointer_result;
        std::string json_pointer = std::string("/") + std::string(key);
        auto error = at_pointer(json_pointer).get(pointer_result);

        if (!error) {
          child_values.emplace_back(pointer_result);
        }
      }

      std::vector<element> result = {};

      if (child_values.size() > 0) {
        std::vector<element>::iterator child_values_begin = child_values.begin();
        std::vector<element>::iterator child_values_end = child_values.end();

        process_json_path_of_child_elements(child_values_begin, child_values_end, json_path, result);
      }

      return result;
    } else {
      return INVALID_JSON_POINTER;
    }
  } else {
    element result;
    auto error = at_path(json_path).get(result);
    if (error) {
      return error;
    }

    return std::vector<element>{std::move(result)};
  }
}

inline simdjson_result<element> array::at(size_t index) const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  size_t i=0;
  for (auto element : *this) {
    if (i == index) { return element; }
    i++;
  }
  return INDEX_OUT_OF_BOUNDS;
}

inline std::vector<element>& array::get_values(std::vector<element>& out) const noexcept {
  out.reserve(this->size());
  for (auto element : *this) {
    out.emplace_back(element);
  }

  return out;
}

inline array::operator element() const noexcept {
	return element(tape);
}

//
// array::iterator inline implementation
//
simdjson_inline array::iterator::iterator(const internal::tape_ref &_tape) noexcept : tape{_tape} { }
inline element array::iterator::operator*() const noexcept {
  return element(tape);
}
inline array::iterator& array::iterator::operator++() noexcept {
  tape.json_index = tape.after_element();
  return *this;
}
inline array::iterator array::iterator::operator++(int) noexcept {
  array::iterator out = *this;
  ++*this;
  return out;
}
inline bool array::iterator::operator!=(const array::iterator& other) const noexcept {
  return tape.json_index != other.tape.json_index;
}
inline bool array::iterator::operator==(const array::iterator& other) const noexcept {
  return tape.json_index == other.tape.json_index;
}
inline bool array::iterator::operator<(const array::iterator& other) const noexcept {
  return tape.json_index < other.tape.json_index;
}
inline bool array::iterator::operator<=(const array::iterator& other) const noexcept {
  return tape.json_index <= other.tape.json_index;
}
inline bool array::iterator::operator>=(const array::iterator& other) const noexcept {
  return tape.json_index >= other.tape.json_index;
}
inline bool array::iterator::operator>(const array::iterator& other) const noexcept {
  return tape.json_index > other.tape.json_index;
}

} // namespace dom


} // namespace simdjson

#include "simdjson/dom/element-inl.h"

#if SIMDJSON_SUPPORTS_RANGES
static_assert(std::ranges::view<simdjson::dom::array>);
static_assert(std::ranges::sized_range<simdjson::dom::array>);
#if SIMDJSON_EXCEPTIONS
static_assert(std::ranges::view<simdjson::simdjson_result<simdjson::dom::array>>);
static_assert(std::ranges::sized_range<simdjson::simdjson_result<simdjson::dom::array>>);
#endif // SIMDJSON_EXCEPTIONS
#endif // SIMDJSON_SUPPORTS_RANGES

#endif // SIMDJSON_ARRAY_INL_H
