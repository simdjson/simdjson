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

simdjson_really_inline array::array(json_iterator_ref &&_iter) noexcept
  : iter{std::forward<json_iterator_ref>(_iter)}
{
}

simdjson_really_inline array::~array() noexcept {
  if (iter.is_alive()) {
    logger::log_event(*iter, "unfinished", "array");
    simdjson_unused auto _err = iter->skip_container();
    iter.release();
  }
}

simdjson_really_inline simdjson_result<array> array::start(json_iterator_ref &&iter) noexcept {
  bool has_value;
  SIMDJSON_TRY( iter->start_array().get(has_value) );
  if (!has_value) { iter.release(); }
  return array(std::forward<json_iterator_ref>(iter));
}
simdjson_really_inline array array::started(json_iterator_ref &&iter) noexcept {
  if (!iter->started_array()) { iter.release(); }
  return array(std::forward<json_iterator_ref>(iter));
}

//
// For array_iterator
//
simdjson_really_inline json_iterator &array::get_iterator() noexcept {
  return *iter;
}
simdjson_really_inline json_iterator_ref array::borrow_iterator() noexcept {
  return iter.borrow();
}
simdjson_really_inline bool array::is_iterator_alive() const noexcept {
  return iter.is_alive();
}
simdjson_really_inline void array::iteration_finished() noexcept {
  iter.release();
}

simdjson_really_inline array_iterator<array> array::begin() & noexcept {
  return *this;
}
simdjson_really_inline array_iterator<array> array::end() & noexcept {
  return {};
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::array &&value
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::array>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::simdjson_result(
  error_code error
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array>(error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<SIMDJSON_IMPLEMENTATION::ondemand::array>> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::begin() & noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<SIMDJSON_IMPLEMENTATION::ondemand::array>> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::end() & noexcept {
  if (error()) { return error(); }
  return first.end();
}

} // namespace simdjson
