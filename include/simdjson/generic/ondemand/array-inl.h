#ifndef SIMDJSON_GENERIC_ONDEMAND_ARRAY_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_ARRAY_INL_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/array.h"
#include "simdjson/generic/ondemand/array_iterator-inl.h"
#include "simdjson/generic/ondemand/json_iterator.h"
#include "simdjson/generic/ondemand/value.h"
#include "simdjson/generic/ondemand/value_iterator-inl.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

//
// ### Live States
//
// While iterating or looking up values, depth >= iter->depth. at_start may vary. Error is
// always SUCCESS:
//
// - Start: This is the state when the array is first found and the iterator is just past the `{`.
//   In this state, at_start == true.
// - Next: After we hand a scalar value to the user, or an array/object which they then fully
//   iterate over, the iterator is at the `,` before the next value (or `]`). In this state,
//   depth == iter->depth, at_start == false, and error == SUCCESS.
// - Unfinished Business: When we hand an array/object to the user which they do not fully
//   iterate over, we need to finish that iteration by skipping child values until we reach the
//   Next state. In this state, depth > iter->depth, at_start == false, and error == SUCCESS.
//
// ## Error States
//
// In error states, we will yield exactly one more value before stopping. iter->depth == depth
// and at_start is always false. We decrement after yielding the error, moving to the Finished
// state.
//
// - Chained Error: When the array iterator is part of an error chain--for example, in
//   `for (auto tweet : doc["tweets"])`, where the tweet element may be missing or not be an
//   array--we yield that error in the loop, exactly once. In this state, error != SUCCESS and
//   iter->depth == depth, and at_start == false. We decrement depth when we yield the error.
// - Missing Comma Error: When the iterator ++ method discovers there is no comma between elements,
//   we flag that as an error and treat it exactly the same as a Chained Error. In this state,
//   error == TAPE_ERROR, iter->depth == depth, and at_start == false.
//
// ## Terminal State
//
// The terminal state has iter->depth < depth. at_start is always false.
//
// - Finished: When we have reached a `]` or have reported an error, we are finished. We signal this
//   by decrementing depth. In this state, iter->depth < depth, at_start == false, and
//   error == SUCCESS.
//

simdjson_inline array::array(const value_iterator &_iter) noexcept
  : iter{_iter}
{
}

simdjson_inline simdjson_result<array> array::start(value_iterator &iter) noexcept {
  // We don't need to know if the array is empty to start iteration, but we do want to know if there
  // is an error--thus `simdjson_unused`.
  simdjson_unused bool has_value;
  SIMDJSON_TRY( iter.start_array().get(has_value) );
  return array(iter);
}
simdjson_inline simdjson_result<array> array::start_root(value_iterator &iter) noexcept {
  simdjson_unused bool has_value;
  SIMDJSON_TRY( iter.start_root_array().get(has_value) );
  return array(iter);
}
simdjson_inline simdjson_result<array> array::started(value_iterator &iter) noexcept {
  bool has_value;
  SIMDJSON_TRY(iter.started_array().get(has_value));
  return array(iter);
}

simdjson_inline simdjson_result<array_iterator> array::begin() noexcept {
#if SIMDJSON_DEVELOPMENT_CHECKS
  if (!iter.is_at_iterator_start()) { return OUT_OF_ORDER_ITERATION; }
#endif
  return array_iterator(iter);
}
simdjson_inline simdjson_result<array_iterator> array::end() noexcept {
  return array_iterator(iter);
}
simdjson_inline error_code array::consume() noexcept {
  auto error = iter.json_iter().skip_child(iter.depth()-1);
  if(error) { iter.abandon(); }
  return error;
}

simdjson_inline simdjson_result<std::string_view> array::raw_json() noexcept {
  const uint8_t * starting_point{iter.peek_start()};
  auto error = consume();
  if(error) { return error; }
  // After 'consume()', we could be left pointing just beyond the document, but that
  // is ok because we are not going to dereference the final pointer position, we just
  // use it to compute the length in bytes.
  const uint8_t * final_point{iter._json_iter->unsafe_pointer()};
  return std::string_view(reinterpret_cast<const char*>(starting_point), size_t(final_point - starting_point));
}

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING
simdjson_inline simdjson_result<size_t> array::count_elements() & noexcept {
  size_t count{0};
  // Important: we do not consume any of the values.
  for(simdjson_unused auto v : *this) { count++; }
  // The above loop will always succeed, but we want to report errors.
  if(iter.error()) { return iter.error(); }
  // We need to move back at the start because we expect users to iterate through
  // the array after counting the number of elements.
  iter.reset_array();
  return count;
}
SIMDJSON_POP_DISABLE_WARNINGS

simdjson_inline simdjson_result<bool> array::is_empty() & noexcept {
  bool is_not_empty;
  auto error = iter.reset_array().get(is_not_empty);
  if(error) { return error; }
  return !is_not_empty;
}

inline simdjson_result<bool> array::reset() & noexcept {
  return iter.reset_array();
}

inline simdjson_result<value> array::at_pointer(std::string_view json_pointer) noexcept {
  if (json_pointer[0] != '/') { return INVALID_JSON_POINTER; }
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
  auto child = at(array_index);
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

simdjson_inline simdjson_result<value> array::at(size_t index) noexcept {
  size_t i = 0;
  for (auto value : *this) {
    if (i == index) { return value; }
    i++;
  }
  return INDEX_OUT_OF_BOUNDS;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::array &&value
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::array>(value)
    )
{
}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::simdjson_result(
  error_code error
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array>(error)
{
}

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::begin() noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::end() noexcept {
  if (error()) { return error(); }
  return first.end();
}
simdjson_inline  simdjson_result<size_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::count_elements() & noexcept {
  if (error()) { return error(); }
  return first.count_elements();
}
simdjson_inline  simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::is_empty() & noexcept {
  if (error()) { return error(); }
  return first.is_empty();
}
simdjson_inline  simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::at(size_t index) noexcept {
  if (error()) { return error(); }
  return first.at(index);
}
simdjson_inline  simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::at_pointer(std::string_view json_pointer) noexcept {
  if (error()) { return error(); }
  return first.at_pointer(json_pointer);
}
simdjson_inline  simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::raw_json() noexcept {
  if (error()) { return error(); }
  return first.raw_json();
}
} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_ARRAY_INL_H