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
inline dom::array::reverse_iterator simdjson_result<dom::array>::rbegin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.rbegin();
}
inline dom::array::reverse_iterator simdjson_result<dom::array>::rend() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.rend();
}

#endif // SIMDJSON_EXCEPTIONS

inline simdjson_result<dom::element> simdjson_result<dom::array>::at_pointer(std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at_pointer(json_pointer);
}

 inline simdjson_result<dom::element> simdjson_result<dom::array>::at_path(std::string_view json_path) const noexcept {
  if (error()) { return error(); }
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
  if (error()) { return out; }
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
inline array::reverse_iterator array::rbegin() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable());
  const internal::tape_ref end_tape(tape.doc, tape.matching_brace_index() - 1);
  return reverse_iterator(internal::tape_ref(tape.doc, end_tape.before_element(tape.json_index)), tape.json_index);
}
inline array::reverse_iterator array::rend() const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable());
  return reverse_iterator(tape, tape.json_index);
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

  size_t array_index = 0;
  size_t i;
  SIMDJSON_TRY(internal::parse_json_pointer_array_index(json_pointer, array_index, i));

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
    accumulator.insert(accumulator.end(),
                        std::make_move_iterator(child_result.begin()),
                        std::make_move_iterator(child_result.end()));
  }
}

inline simdjson_result<std::vector<element>> array::at_path_with_wildcard(std::string_view json_path) const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(tape.usable()); // https://github.com/simdjson/simdjson/issues/1914
  return element(*this).at_path_with_wildcard(json_path);
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
// array::reverse_iterator inline implementation
//
simdjson_inline array::reverse_iterator::reverse_iterator(const internal::tape_ref &_tape, size_t _array_start) noexcept
    : tape{_tape}, array_start{_array_start} { }
inline element array::reverse_iterator::operator*() const noexcept {
  return element(tape);
}
inline array::reverse_iterator& array::reverse_iterator::operator++() noexcept {
  tape.json_index = tape.before_element(array_start);
  return *this;
}
inline array::reverse_iterator array::reverse_iterator::operator++(int) noexcept {
  reverse_iterator out = *this;
  ++*this;
  return out;
}
inline bool array::reverse_iterator::operator==(const reverse_iterator& other) const noexcept {
  return tape.doc == other.tape.doc && tape.json_index == other.tape.json_index;
}
inline bool array::reverse_iterator::operator!=(const reverse_iterator& other) const noexcept {
  return !(*this == other);
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
