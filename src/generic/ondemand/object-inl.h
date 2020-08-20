namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

//
// ### Live States
//
// While iterating or looking up values, depth >= doc->iter.depth. at_start may vary. Error is
// always SUCCESS:
//
// - Start: This is the state when the object is first found and the iterator is just past the {.
//   In this state, at_start == true.
// - Next: After we hand a scalar value to the user, or an array/object which they then fully
//   iterate over, the iterator is at the , or } before the next value. In this state,
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
// - Chained Error: When the object iterator is part of an error chain--for example, in
//   `for (auto tweet : doc["tweets"])`, where the tweet field may be missing or not be an
//   object--we yield that error in the loop, exactly once. In this state, error != SUCCESS and
//   doc->iter.depth == depth, and at_start == false. We decrement depth when we yield the error.
// - Missing Comma Error: When the iterator ++ method discovers there is no comma between fields,
//   we flag that as an error and treat it exactly the same as a Chained Error. In this state,
//   error == TAPE_ERROR, doc->iter.depth == depth, and at_start == false.
//
// Errors that occur while reading a field to give to the user (such as when the key is not a
// string or the field is missing a colon) are yielded immediately. Depth is then decremented,
// moving to the Finished state without transitioning through an Error state at all.
//
// ## Terminal State
//
// The terminal state has doc->iter.depth < depth. at_start is always false.
//
// - Finished: When we have reached a }, we are finished. We signal this by decrementing depth.
//   In this state, doc->iter.depth < depth, at_start == false, and error == SUCCESS.
//

simdjson_really_inline object::object() noexcept = default;
simdjson_really_inline object::object(document *_doc, json_iterator::container _container) noexcept
  : doc{_doc}, container{_container}, at_start{true}, error{SUCCESS}
{
}
simdjson_really_inline object::object(document *_doc, error_code _error) noexcept
  : doc{_doc}, container{_doc->iter.current_container()}, at_start{false}, error{_error}
{
}

simdjson_really_inline bool object::finished() const noexcept {
  return !doc->iter.in_container(container);
}

simdjson_really_inline bool object::check_empty_object() noexcept {
  at_start = false;
  return doc->iter.is_empty_object();
}
simdjson_really_inline error_code object::report_error() noexcept {
  container = doc->iter.current_container().child(); // Make it so we'll stop
  auto result = error;
  error = SUCCESS;
  return result;
}

simdjson_really_inline simdjson_result<value> object::operator[](const std::string_view key) noexcept {
  if (error) { return { doc, report_error() }; }

  error_code _err;
  bool has_next;
  if (at_start) {
    has_next = !check_empty_object();
  } else {
    // If we already finished, don't keep cranking the iterator
    if (finished()) { return { doc, NO_SUCH_FIELD }; }

    // Go to the next field, skipping anything the user 
    if ((_err = doc->iter.next_field(container).get(has_next) )) { return { doc, _err }; }
  }

  while (has_next) {
    // Get the key
    raw_json_string actual_key;
    if ((_err = doc->iter.field_key().get(actual_key) )) { return { doc, _err }; };
    if ((_err = doc->iter.field_value() )) { return { doc, _err }; }

    // Check if it matches
    if (actual_key == key) {
      logger::log_event(doc->iter, "match", key);
      return value::start(doc);
    }
    logger::log_event(doc->iter, "no match", key);
    doc->iter.skip(); // Skip the value entirely
    if ((_err = doc->iter.next_field(container).get(has_next)) ) { return { doc, _err }; }
  }

  // If the loop ended, we're out of fields to look at.
  return { doc, NO_SUCH_FIELD };
}

simdjson_really_inline object object::start(document *doc) noexcept {
  return object(doc, doc->iter.start_object());
}
simdjson_really_inline object object::started(document *doc) noexcept {
  return object(doc, doc->iter.started_object());
}
simdjson_really_inline object object::error_chain(document *doc, error_code error) noexcept {
  return object(doc, error);
}
simdjson_really_inline object object::begin() noexcept {
  return *this;
}
simdjson_really_inline object object::end() noexcept {
  return {};
}

simdjson_really_inline simdjson_result<field> object::operator*() noexcept {
  // For people who use the iterator raw
  if (at_start) { check_empty_object(); }
  // Also for people who use the iterator raw
  if (finished()) {
    logger::log_error(doc->iter, "Attempt to get field from empty object");
    return { doc, NO_SUCH_FIELD };
  }

  if (error) { return { doc, report_error() }; }
  return field::start(doc);
}
simdjson_really_inline bool object::operator==(const object &other) noexcept {
  return !(*this != other);
}
simdjson_really_inline bool object::operator!=(const object &) noexcept {
  // If we're at the start, check for the first field.
  if (at_start) { check_empty_object(); }
  return !finished();
}
simdjson_really_inline object &object::operator++() noexcept {
  SIMDJSON_ASSUME(!error);
  SIMDJSON_ASSUME(!at_start);
  if (finished()) { return *this; } // Only possible when there was an error and we're about to stop.
  
  error = doc->iter.next_field(container).error();
  return *this;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::object &&value) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::object>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::document *doc, error_code error) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object>({ doc, error }, error) {}

simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::object simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::begin() noexcept {
  return first;
}
simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::object simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::end() noexcept {
  return {};
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::operator[](std::string_view key) noexcept {
  if (error()) { return { first.doc, error() }; }
  return first[key];
}

} // namespace simdjson
