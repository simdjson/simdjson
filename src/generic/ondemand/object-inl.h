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
simdjson_really_inline object::object(document *_doc, error_code _error) noexcept
  : doc{_doc}, depth{_doc->iter.depth}, at_start{!_error}, error{_error}
{
}

simdjson_really_inline bool object::finished() const noexcept {
  return doc->iter.depth < depth;
}
simdjson_really_inline void object::finish(bool log_end) noexcept {
  doc->iter.depth = depth - 1;
  if (log_end) { logger::log_end_value(doc->iter, "object"); }
}

simdjson_really_inline void object::first_field() noexcept {
  at_start = false;
  // If it's empty, shut down
  if (*doc->iter.peek() == '}') {
    logger::log_value(doc->iter, "empty object", "", -1, -1);
    doc->iter.advance();
    finish();
  } else {
    logger::log_start_value(doc->iter, "object", -1, -1);
  }
}
simdjson_really_inline simdjson_result<value> object::operator[](const std::string_view key) noexcept {
  if (finished()) { return { doc, NO_SUCH_FIELD }; }
  if (error) { finish(); return { doc, error }; }

  if (at_start) {
    first_field();
  } else {
    doc->iter.skip_unfinished_children(depth);
    switch (*doc->iter.advance()) {
      case ',':
        break;
      case '}':
        finish(true);
        return { doc, NO_SUCH_FIELD };
      default:
        logger::log_error(doc->iter, "Missing comma between object fields");
        finish();
        return { doc, TAPE_ERROR };
    }
  }

  while (true) {
    const uint8_t *actual_key = doc->iter.advance();
    switch (*(actual_key++)) {
      case '"':
        if (raw_json_string(actual_key) == key) {
          logger::log_event(doc->iter, "match", key);
          return field::start_value(doc);
        }
        logger::log_event(doc->iter, "no match", key);
        doc->iter.advance(); // "key" :
        doc->iter.skip_value(); // "key" : <value>
        switch (*doc->iter.advance()) {
          case ',':
            break;
          case '}':
            logger::log_event(doc->iter, "no key found", key);
            finish(true);
            return { doc, NO_SUCH_FIELD };
          default:
            logger::log_error(doc->iter, "Missing comma between object fields");
            finish();
            return { doc, TAPE_ERROR };
        }
        break;
      default:
        logger::log_error(doc->iter, "Key is not a string");
        finish();
        return { doc, TAPE_ERROR };
    }
  }
}

simdjson_really_inline object object::begin(document *doc, error_code error) noexcept {
  doc->iter.depth++;
  return object(doc, error);
}
simdjson_really_inline object object::begin() noexcept {
  return *this;
}
simdjson_really_inline object object::end() noexcept {
  return {};
}

simdjson_really_inline simdjson_result<field> object::operator*() noexcept {
  if (error) { finish(); return { doc, error }; }
  return field::start(doc);
}
simdjson_really_inline bool object::operator==(const object &other) noexcept {
  return !(*this != other);
}
simdjson_really_inline bool object::operator!=(const object &) noexcept {
  // If we're at the start, check for the first field.
  if (at_start) { first_field(); }
  return !finished();
}
simdjson_really_inline object &object::operator++() noexcept {
  if (!finished()) {
    SIMDJSON_ASSUME(!error);
    SIMDJSON_ASSUME(!at_start);
    doc->iter.skip_unfinished_children(depth);
    switch (*doc->iter.advance()) {
      case ',':
        break;
      case '}':
        finish(true);
        break;
      default:
        logger::log_error(doc->iter, "Missing comma between object fields");
        finish();
        error = TAPE_ERROR;
    }
  }
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
