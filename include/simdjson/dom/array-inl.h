#ifndef SIMDJSON_INLINE_ARRAY_H
#define SIMDJSON_INLINE_ARRAY_H

// Inline implementations go in here.

#include "simdjson/dom/array.h"
#include "simdjson/dom/element.h"
#include <utility>

namespace simdjson {

//
// simdjson_result<dom::array> inline implementation
//
simdjson_really_inline simdjson_result<dom::array>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::array>() {}
simdjson_really_inline simdjson_result<dom::array>::simdjson_result(dom::array value) noexcept
    : internal::simdjson_result_base<dom::array>(std::forward<dom::array>(value)) {}
simdjson_really_inline simdjson_result<dom::array>::simdjson_result(error_code error) noexcept
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
inline simdjson_result<dom::element> simdjson_result<dom::array>::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}

namespace dom {

//
// array inline implementation
//
simdjson_really_inline array::array() noexcept : tape{} {}
simdjson_really_inline array::array(const internal::tape_ref &_tape) noexcept : tape{_tape} {}
inline array::iterator array::begin() const noexcept {
  return internal::tape_ref(tape.doc, tape.json_index + 1);
}
inline array::iterator array::end() const noexcept {
  return internal::tape_ref(tape.doc, tape.after_element() - 1);
}
inline size_t array::size() const noexcept {
  return tape.scope_count();
}
inline simdjson_result<element> array::at_pointer(std::string_view json_pointer) const noexcept {
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

inline simdjson_result<element> array::at(size_t index) const noexcept {
  size_t i=0;
  for (auto element : *this) {
    if (i == index) { return element; }
    i++;
  }
  return INDEX_OUT_OF_BOUNDS;
}

//
// array::iterator inline implementation
//
simdjson_really_inline array::iterator::iterator(const internal::tape_ref &_tape) noexcept : tape{_tape} { }
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
inline std::ostream& operator<<(std::ostream& out, const array &value) {
  return out << minify<array>(value);
}

} // namespace dom

template<>
inline std::ostream& minifier<dom::array>::print(std::ostream& out) {
  out << '[';
  auto iter = value.begin();
  auto end = value.end();
  if (iter != end) {
    out << minify<dom::element>(*iter);
    for (++iter; iter != end; ++iter) {
      out << "," << minify<dom::element>(*iter);
    }
  }
  return out << ']';
}

#if SIMDJSON_EXCEPTIONS

template<>
inline std::ostream& minifier<simdjson_result<dom::array>>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<dom::array>(value.first);
}

inline std::ostream& operator<<(std::ostream& out, const simdjson_result<dom::array> &value) noexcept(false) {
  return out << minify<simdjson_result<dom::array>>(value);
}

#endif

} // namespace simdjson

#include "simdjson/dom/element-inl.h"

#if defined(__cpp_lib_ranges)
static_assert(std::ranges::view<simdjson::dom::array>);
static_assert(std::ranges::sized_range<simdjson::dom::array>);
#if SIMDJSON_EXCEPTIONS
static_assert(std::ranges::view<simdjson::simdjson_result<simdjson::dom::array>>);
static_assert(std::ranges::sized_range<simdjson::simdjson_result<simdjson::dom::array>>);
#endif // SIMDJSON_EXCEPTIONS
#endif // defined(__cpp_lib_ranges)

#endif // SIMDJSON_INLINE_ARRAY_H
