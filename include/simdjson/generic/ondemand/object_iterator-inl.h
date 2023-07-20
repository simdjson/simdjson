#ifndef SIMDJSON_GENERIC_ONDEMAND_OBJECT_ITERATOR_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_OBJECT_ITERATOR_INL_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/object_iterator.h"
#include "simdjson/generic/ondemand/field-inl.h"
#include "simdjson/generic/ondemand/value_iterator-inl.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

//
// object_iterator
//

simdjson_inline object_iterator::object_iterator(const value_iterator &_iter) noexcept
  : iter{_iter}
{}

simdjson_inline simdjson_result<field> object_iterator::operator*() noexcept {
  error_code error = iter.error();
  if (error) { iter.abandon(); return error; }
  auto result = field::start(iter);
  // TODO this is a safety rail ... users should exit loops as soon as they receive an error.
  // Nonetheless, let's see if performance is OK with this if statement--the compiler may give it to us for free.
  if (result.error()) { iter.abandon(); }
  return result;
}
simdjson_inline bool object_iterator::operator==(const object_iterator &other) const noexcept {
  return !(*this != other);
}
simdjson_inline bool object_iterator::operator!=(const object_iterator &) const noexcept {
  return iter.is_open();
}

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING
simdjson_inline object_iterator &object_iterator::operator++() noexcept {
  // TODO this is a safety rail ... users should exit loops as soon as they receive an error.
  // Nonetheless, let's see if performance is OK with this if statement--the compiler may give it to us for free.
  if (!iter.is_open()) { return *this; } // Iterator will be released if there is an error

  simdjson_unused error_code error;
  if ((error = iter.skip_child() )) { return *this; }

  simdjson_unused bool has_value;
  if ((error = iter.has_next_field().get(has_value) )) { return *this; };
  return *this;
}
SIMDJSON_POP_DISABLE_WARNINGS

//
// ### Live States
//
// While iterating or looking up values, depth >= iter.depth. at_start may vary. Error is
// always SUCCESS:
//
// - Start: This is the state when the object is first found and the iterator is just past the {.
//   In this state, at_start == true.
// - Next: After we hand a scalar value to the user, or an array/object which they then fully
//   iterate over, the iterator is at the , or } before the next value. In this state,
//   depth == iter.depth, at_start == false, and error == SUCCESS.
// - Unfinished Business: When we hand an array/object to the user which they do not fully
//   iterate over, we need to finish that iteration by skipping child values until we reach the
//   Next state. In this state, depth > iter.depth, at_start == false, and error == SUCCESS.
//
// ## Error States
//
// In error states, we will yield exactly one more value before stopping. iter.depth == depth
// and at_start is always false. We decrement after yielding the error, moving to the Finished
// state.
//
// - Chained Error: When the object iterator is part of an error chain--for example, in
//   `for (auto tweet : doc["tweets"])`, where the tweet field may be missing or not be an
//   object--we yield that error in the loop, exactly once. In this state, error != SUCCESS and
//   iter.depth == depth, and at_start == false. We decrement depth when we yield the error.
// - Missing Comma Error: When the iterator ++ method discovers there is no comma between fields,
//   we flag that as an error and treat it exactly the same as a Chained Error. In this state,
//   error == TAPE_ERROR, iter.depth == depth, and at_start == false.
//
// Errors that occur while reading a field to give to the user (such as when the key is not a
// string or the field is missing a colon) are yielded immediately. Depth is then decremented,
// moving to the Finished state without transitioning through an Error state at all.
//
// ## Terminal State
//
// The terminal state has iter.depth < depth. at_start is always false.
//
// - Finished: When we have reached a }, we are finished. We signal this by decrementing depth.
//   In this state, iter.depth < depth, at_start == false, and error == SUCCESS.
//

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::object_iterator &&value
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>(value))
{
  first.iter.assert_is_valid();
}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::simdjson_result(error_code error) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>({}, error)
{
}

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::operator*() noexcept {
  if (error()) { return error(); }
  return *first;
}
// If we're iterating and there is an error, return the error once.
simdjson_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &other) const noexcept {
  if (!first.iter.is_valid()) { return !error(); }
  return first == other.first;
}
// If we're iterating and there is an error, return the error once.
simdjson_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &other) const noexcept {
  if (!first.iter.is_valid()) { return error(); }
  return first != other.first;
}
// Checks for ']' and ','
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::operator++() noexcept {
  // Clear the error if there is one, so we don't yield it twice
  if (error()) { second = SUCCESS; return *this; }
  ++first;
  return *this;
}

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_OBJECT_ITERATOR_INL_H