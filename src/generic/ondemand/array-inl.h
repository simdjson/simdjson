namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

//
// ### Live States
//
// While iterating or looking up values, depth >= doc->iter.depth. at_start may vary. Error is
// always SUCCESS:
//
// - Start: This is the state when the array is first found and the iterator is just past the `{`.
//   In this state, at_start == true.
// - Next: After we hand a scalar value to the user, or an array/object which they then fully
//   iterate over, the iterator is at the `,` before the next value (or `]`). In this state,
//   depth == doc->iter.depth, at_start == false, and error == SUCCESS.
// - Unfinished Business: When we hand an array/object to the user which they do not fully
//   iterate over, we need to finish that iteration by skipping child values until we reach the
//   Next state. In this state, depth > doc->iter.depth, at_start == false, and error == SUCCESS.
//
// ## Error States
// 
// In error states, we will yield exactly one more value before stopping. doc->iter.depth == depth
// and at_start is always false. We decrement after yielding the error, moving to the Finished
// state.
//
// - Chained Error: When the array iterator is part of an error chain--for example, in
//   `for (auto tweet : doc["tweets"])`, where the tweet element may be missing or not be an
//   array--we yield that error in the loop, exactly once. In this state, error != SUCCESS and
//   doc->iter.depth == depth, and at_start == false. We decrement depth when we yield the error.
// - Missing Comma Error: When the iterator ++ method discovers there is no comma between elements,
//   we flag that as an error and treat it exactly the same as a Chained Error. In this state,
//   error == TAPE_ERROR, doc->iter.depth == depth, and at_start == false.
//
// ## Terminal State
//
// The terminal state has doc->iter.depth < depth. at_start is always false.
//
// - Finished: When we have reached a `]` or have reported an error, we are finished. We signal this
//   by decrementing depth. In this state, doc->iter.depth < depth, at_start == false, and
//   error == SUCCESS.
//

simdjson_really_inline array::array() noexcept = default;
simdjson_really_inline array::array(document *_doc, json_iterator::container _container) noexcept
  : doc{_doc}, container{_container}, at_start{true}, error{SUCCESS}
{
}
simdjson_really_inline array::array(document *_doc, error_code _error) noexcept
  : doc{_doc}, container{_doc->iter.current_container()}, at_start{false}, error{_error}
{
  SIMDJSON_ASSUME(_error);
}

simdjson_really_inline bool array::finished() const noexcept {
  return !doc->iter.in_container(container);
}

simdjson_really_inline array array::start(document *doc) noexcept {
  return array(doc, doc->iter.start_array());
}
simdjson_really_inline array array::started(document *doc) noexcept {
  return array(doc, doc->iter.started_array());
}
simdjson_really_inline array array::error_chain(document *doc, error_code error) noexcept {
  return array(doc, error);
}
simdjson_really_inline array array::begin() noexcept {
  return *this;
}
simdjson_really_inline array array::end() noexcept {
  return {};
}

simdjson_really_inline error_code array::report_error() noexcept {
  container = doc->iter.current_container().child(); // Make it so we'll stop
  auto result = error;
  error = SUCCESS;
  return result;
}

simdjson_really_inline simdjson_result<value> array::operator*() noexcept {
  if (error) { return { doc, report_error() }; }
  return value::start(doc);
}
simdjson_really_inline bool array::operator==(const array &other) noexcept {
  return !(*this != other);
}
simdjson_really_inline bool array::operator!=(const array &) noexcept {
  // If we're at the start, check for empty array.
  if (at_start) {
    at_start = false;
    return !doc->iter.is_empty_array();
  }
  return !finished();
}
simdjson_really_inline array &array::operator++() noexcept {
  SIMDJSON_ASSUME(!finished());
  SIMDJSON_ASSUME(!at_start);

  error = doc->iter.next_element(container).error();
  return *this;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::array &&value
) noexcept :
  internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array>(
    std::forward<SIMDJSON_IMPLEMENTATION::ondemand::array>(value)
  )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::document *doc,
  error_code error
) noexcept :
  internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array>({ doc, error }, error)
{
}

simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::array simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::begin() noexcept {
  return first;
}
simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::array simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::end() noexcept {
  return {};
}

} // namespace simdjson
