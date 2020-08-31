namespace {
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

simdjson_really_inline array::array() noexcept = default;
simdjson_really_inline array::array(json_iterator_ref &&_iter, bool has_value) noexcept
  : iter{std::forward<json_iterator_ref>(_iter)}, has_next{has_value}, error{SUCCESS}
{
}
simdjson_really_inline array::array(array &&other) noexcept
  : iter{std::forward<array>(other).iter}, has_next{other.has_next}, error{other.error}
{
  // Terminate the other iterator
  other.has_next = false;
}
simdjson_really_inline array &array::operator=(array &&other) noexcept {
  iter = std::forward<array>(other).iter;
  has_next = other.has_next;
  error = other.error;
  // Terminate the other iterator
  other.has_next = false;
  return *this;
}

simdjson_really_inline array::~array() noexcept {
  if (!error && has_next && iter.is_alive()) {
    logger::log_event(*iter, "unfinished", "array");
    iter->skip_container();
    iter.release();
  }
}

simdjson_really_inline simdjson_result<array> array::start(json_iterator_ref &&iter) noexcept {
  bool has_value;
  SIMDJSON_TRY( iter->start_array().get(has_value) );
  return array(std::forward<json_iterator_ref>(iter), has_value);
}
simdjson_really_inline array array::started(json_iterator_ref &&iter) noexcept {
  return array(std::forward<json_iterator_ref>(iter), iter->started_array());
}
simdjson_really_inline array::iterator array::begin() noexcept {
  return *this;
}
simdjson_really_inline array::iterator array::end() noexcept {
  return *this;
}

simdjson_really_inline error_code array::report_error() noexcept {
  SIMDJSON_ASSUME(error);
  has_next = false;
  return error;
}

simdjson_really_inline array::iterator::iterator(array &_a) noexcept : a{&_a} {}

simdjson_really_inline array::iterator::iterator() noexcept = default;
simdjson_really_inline array::iterator::iterator(const array::iterator &_a) noexcept = default;
simdjson_really_inline array::iterator &array::iterator::operator=(const array::iterator &_a) noexcept = default;

simdjson_really_inline simdjson_result<value> array::iterator::operator*() noexcept {
  if (a->error) { return a->report_error(); }
  return value::start(a->iter.borrow());
}
simdjson_really_inline bool array::iterator::operator==(const array::iterator &) noexcept {
  return !a->has_next;
}
simdjson_really_inline bool array::iterator::operator!=(const array::iterator &) noexcept {
  return a->has_next;
}
simdjson_really_inline array::iterator &array::iterator::operator++() noexcept {
  if (a->error) { return *this; }
  a->error = a->iter->has_next_element().get(a->has_next); // If there's an error, has_next stays true.
  return *this;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::array &&value
) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::array>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::simdjson_result(
  error_code error
) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array>(error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::begin() noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::end() noexcept {
  if (error()) { return error(); }
  return first.end();
}

//
// array::iterator
//

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>::simdjson_result() noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>({}, SUCCESS)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::array::iterator &&value
) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>(value))
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>::simdjson_result(error_code error) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>({}, error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>::simdjson_result(
  const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &a
) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>(a)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>::operator=(
  const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &a
) noexcept {
  first = a.first;
  second = a.second;
  return *this;
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>::operator*() noexcept {
  if (error()) { return error(); }
  return *first;
}
// Assumes it's being compared with the end. true if depth < iter->depth.
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>::operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &other) noexcept {
  if (error()) { return true; }
  return first == other.first;
}
// Assumes it's being compared with the end. true if depth >= iter->depth.
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>::operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &other) noexcept {
  if (error()) { return false; }
  return first != other.first;
}
// Checks for ']' and ','
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator>::operator++() noexcept {
  if (error()) { return *this; }
  ++first;
  return *this;
}

} // namespace simdjson
