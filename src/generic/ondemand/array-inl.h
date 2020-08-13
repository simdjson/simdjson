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
simdjson_really_inline array::array(document *_doc, error_code _error) noexcept
  : doc{_doc}, depth{_doc->iter.depth}, at_start{!_error}, error{_error}
{
}

simdjson_really_inline bool array::finished() const noexcept {
  return doc->iter.depth < depth;
}
simdjson_really_inline void array::finish(bool log_end) noexcept {
  doc->iter.depth = depth - 1;
  if (log_end) { logger::log_end_value(doc->iter, "array"); }
}

simdjson_really_inline array array::begin(document *doc, error_code error) noexcept {
  doc->iter.depth++;
  return array(doc, error);
}
simdjson_really_inline array array::begin() noexcept {
  return *this;
}
simdjson_really_inline array array::end() noexcept {
  return {};
}

simdjson_really_inline simdjson_result<value> array::operator*() noexcept {
  if (error) { finish(); return { doc, error }; }
  return value::start(doc);
}
simdjson_really_inline bool array::operator==(const array &other) noexcept {
  return !(*this != other);
}
simdjson_really_inline bool array::operator!=(const array &) noexcept {
  // If we're at the start, check for empty array.
  if (at_start) {
    if (*doc->iter.peek() == ']') {
      doc->iter.advance();
      logger::log_value(doc->iter, "empty array");
      finish();
    } else {
      logger::log_start_value(doc->iter, "array");
    }
  }
  return !finished();
}
simdjson_really_inline array &array::operator++() noexcept {
  if (!finished()) {
    SIMDJSON_ASSUME(!error);
    SIMDJSON_ASSUME(!at_start);
    doc->iter.skip_unfinished_children(depth);
    switch (*doc->iter.advance()) {
      case ',':
        break;
      case ']':
        finish(true);
        break;
      default:
        logger::log_error(doc->iter, "Missing comma between array elements");
        finish();
        error = TAPE_ERROR;
    }
  }
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
