namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

//
// ### Live States
//
// While iterating or looking up values, depth >= iter->depth. at_start may vary. Error is
// always SUCCESS:
//
// - Start: This is the state when the object is first found and the iterator is just past the {.
//   In this state, at_start == true.
// - Next: After we hand a scalar value to the user, or an array/object which they then fully
//   iterate over, the iterator is at the , or } before the next value. In this state,
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
// - Chained Error: When the object iterator is part of an error chain--for example, in
//   `for (auto tweet : doc["tweets"])`, where the tweet field may be missing or not be an
//   object--we yield that error in the loop, exactly once. In this state, error != SUCCESS and
//   iter->depth == depth, and at_start == false. We decrement depth when we yield the error.
// - Missing Comma Error: When the iterator ++ method discovers there is no comma between fields,
//   we flag that as an error and treat it exactly the same as a Chained Error. In this state,
//   error == TAPE_ERROR, iter->depth == depth, and at_start == false.
//
// Errors that occur while reading a field to give to the user (such as when the key is not a
// string or the field is missing a colon) are yielded immediately. Depth is then decremented,
// moving to the Finished state without transitioning through an Error state at all.
//
// ## Terminal State
//
// The terminal state has iter->depth < depth. at_start is always false.
//
// - Finished: When we have reached a }, we are finished. We signal this by decrementing depth.
//   In this state, iter->depth < depth, at_start == false, and error == SUCCESS.
//

simdjson_really_inline object::object() noexcept = default;
simdjson_really_inline object::object(json_iterator_ref &&_iter, bool _has_value) noexcept
  : iter{std::forward<json_iterator_ref>(_iter)}, has_next{_has_value}, at_start{true}, error{SUCCESS}
{
}
simdjson_really_inline object::object(object &&other) noexcept
  : iter{std::forward<object>(other).iter}, has_next{other.has_next}, at_start{other.at_start}, error{other.error}
{
  // Terminate the other iterator
  other.has_next = false;
}
simdjson_really_inline object &object::operator=(object &&other) noexcept {
  iter = std::forward<object>(other).iter;
  has_next = other.has_next;
  at_start = other.at_start;
  error = other.error;
  // Terminate the other iterator
  other.has_next = false;
  return *this;
}

simdjson_really_inline object::~object() noexcept {
  if (!error && has_next && iter.is_alive()) {
    logger::log_event(*iter, "unfinished", "object");
    iter->skip_container();
    iter.release();
  }
}

simdjson_really_inline simdjson_result<value> object::operator[](const std::string_view key) noexcept {
  if (error) { return report_error(); }

  // Unless this is the first field, we need to advance past the , and check for }
  if (has_next) {
    if (at_start) {
      at_start = false;
    } else {
      if ((error = iter->has_next_field().get(has_next) )) { return report_error(); }
    }
  }
  while (has_next) {
    // Get the key
    raw_json_string actual_key;
    if ((error = iter->field_key().get(actual_key) )) { return report_error(); };
    if ((error = iter->field_value() )) { return report_error(); }

    // Check if it matches
    if (actual_key == key) {
      logger::log_event(*iter, "match", key, -2);
      return value::start(iter.borrow());
    }
    logger::log_event(*iter, "no match", key, -2);
    iter->skip(); // Skip the value entirely
    if ((error = iter->has_next_field().get(has_next) )) { return report_error(); }
    if (!has_next) { iter.release(); }
  }

  // If the loop ended, we're out of fields to look at.
  return NO_SUCH_FIELD;
}

simdjson_really_inline simdjson_result<object> object::start(json_iterator_ref &&iter) noexcept {
  bool has_value;
  SIMDJSON_TRY( iter->start_object().get(has_value) );
  return object(std::forward<json_iterator_ref>(iter), has_value);
}
simdjson_really_inline object object::started(json_iterator_ref &&iter) noexcept {
  return object(std::forward<json_iterator_ref>(iter), iter->started_object());
}
simdjson_really_inline object::iterator object::begin() noexcept {
  return *this;
}
simdjson_really_inline object::iterator object::end() noexcept {
  return *this;
}

simdjson_really_inline error_code object::report_error() noexcept {
  SIMDJSON_ASSUME(error);
  has_next = false;
  iter.release();
  return error;
}

//
// object::iterator
//

simdjson_really_inline object::iterator::iterator(object &_o) noexcept : o{&_o} {}

simdjson_really_inline object::iterator::iterator() noexcept = default;
simdjson_really_inline object::iterator::iterator(const object::iterator &_o) noexcept = default;
simdjson_really_inline object::iterator &object::iterator::operator=(const object::iterator &_o) noexcept = default;

simdjson_really_inline simdjson_result<field> object::iterator::operator*() noexcept {
  if (o->error) { return o->report_error(); }
  if (o->at_start) { o->at_start = false; }
  return field::start(o->iter.borrow());
}
simdjson_really_inline bool object::iterator::operator==(const object::iterator &) noexcept {
  return !o->has_next;
}
simdjson_really_inline bool object::iterator::operator!=(const object::iterator &) noexcept {
  return o->has_next;
}
simdjson_really_inline object::iterator &object::iterator::operator++() noexcept {
  if (o->error) { return *this; }
  o->error = o->iter->has_next_field().get(o->has_next); // If there's an error, has_next stays true.
  if (!o->has_next) { o->iter.release(); }
  return *this;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::object &&value) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::object>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object>(error) {}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::begin() noexcept {
  if (error()) { return error(); }
  return SIMDJSON_IMPLEMENTATION::ondemand::object::iterator(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::end() noexcept {
  if (error()) { return error(); }
  return {};
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::operator[](std::string_view key) noexcept {
  if (error()) { return error(); }
  return first[key];
}

//
// object::iterator
//

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::simdjson_result() noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>({}, SUCCESS)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::object::iterator &&value
) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>(value))
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::simdjson_result(error_code error) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>({}, error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::simdjson_result(
  const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &o
) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>(o)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::operator=(
  const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &o
) noexcept {
  first = o.first;
  second = o.second;
  return *this;
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::operator*() noexcept {
  if (error()) { second = SUCCESS; return error(); }
  return *first;
}
// Assumes it's being compared with the end. true if depth < iter->depth.
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &other) noexcept {
  if (error()) { return true; }
  return first == other.first;
}
// Assumes it's being compared with the end. true if depth >= iter->depth.
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &other) noexcept {
  if (error()) { return false; }
  return first != other.first;
}
// Checks for ']' and ','
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::operator++() noexcept {
  if (error()) { return *this; }
  ++first;
  return *this;
}

} // namespace simdjson
