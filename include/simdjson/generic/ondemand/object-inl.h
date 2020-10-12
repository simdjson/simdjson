namespace simdjson {
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

simdjson_really_inline object::object(json_iterator_ref &&_iter) noexcept
  : iter{std::forward<json_iterator_ref>(_iter)},
    at_start{iter.is_alive()}
{
}


simdjson_really_inline object::~object() noexcept {
  if (iter.is_alive()) {
    logger::log_event(*iter, "unfinished", "object");
    simdjson_unused auto _err = iter->skip_container();
    iter.release();
  }
}

simdjson_really_inline error_code object::find_field(const std::string_view key) noexcept {
  if (!iter.is_alive()) { return NO_SUCH_FIELD; }

  // Unless this is the first field, we need to advance past the , and check for }
  error_code error;
  bool has_value;
  if (at_start) {
    at_start = false;
    has_value = true;
  } else {
    if ((error = iter->has_next_field().get(has_value) )) { iter.release(); return error; }
  }
  while (has_value) {
    // Get the key
    raw_json_string actual_key;
    if ((error = iter->field_key().get(actual_key) )) { iter.release(); return error; };
    if ((error = iter->field_value() )) { iter.release(); return error; }

    // Check if it matches
    if (actual_key == key) {
      logger::log_event(*iter, "match", key, -2);
      return SUCCESS;
    }
    logger::log_event(*iter, "no match", key, -2);
    SIMDJSON_TRY( iter->skip() ); // Skip the value entirely
    if ((error = iter->has_next_field().get(has_value) )) { iter.release(); return error; }
  }

  // If the loop ended, we're out of fields to look at.
  iter.release();
  return NO_SUCH_FIELD;
}

simdjson_really_inline simdjson_result<value> object::operator[](const std::string_view key) & noexcept {
  SIMDJSON_TRY( find_field(key) );
  return value::start(iter.borrow());
}

simdjson_really_inline simdjson_result<value> object::operator[](const std::string_view key) && noexcept {
  SIMDJSON_TRY( find_field(key) );
  return value::start(std::forward<json_iterator_ref>(iter));
}

simdjson_really_inline simdjson_result<object> object::start(json_iterator_ref &&iter) noexcept {
  bool has_value;
  SIMDJSON_TRY( iter->start_object().get(has_value) );
  if (!has_value) { iter.release(); }
  return object(std::forward<json_iterator_ref>(iter));
}
simdjson_really_inline object object::started(json_iterator_ref &&iter) noexcept {
  if (!iter->started_object()) { iter.release(); }
  return object(std::forward<json_iterator_ref>(iter));
}
simdjson_really_inline object_iterator object::begin() noexcept {
  if (at_start) {
    iter.assert_is_active();
  } else {
    iter.assert_is_not_active();
  }
  at_start = false;
  return iter;
}
simdjson_really_inline object_iterator object::end() noexcept {
  return {};
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::object &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::object>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object>(error) {}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::begin() noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::end() noexcept {
  if (error()) { return error(); }
  return first.end();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::operator[](std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::operator[](std::string_view key) && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::object>(first)[key];
}

} // namespace simdjson
