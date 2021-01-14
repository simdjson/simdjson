namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline json_iterator::json_iterator(json_iterator &&other) noexcept
  : token(std::forward<token_iterator>(other.token)),
    parser{other.parser},
    _string_buf_loc{other._string_buf_loc},
    _depth{other._depth}
{
  other.parser = nullptr;
}
simdjson_really_inline json_iterator &json_iterator::operator=(json_iterator &&other) noexcept {
  token = other.token;
  parser = other.parser;
  _string_buf_loc = other._string_buf_loc;
  _depth = other._depth;
  other.parser = nullptr;
  return *this;
}

simdjson_really_inline json_iterator::json_iterator(const uint8_t *buf, ondemand::parser *_parser) noexcept
  : token(buf, _parser->dom_parser->structural_indexes.get()),
    parser{_parser},
    _string_buf_loc{parser->string_buf.get()},
    _depth{1}
{
  // Release the string buf so it can be reused by the next document
  logger::log_headers();
}

// GCC 7 warns when the first line of this function is inlined away into oblivion due to the caller
// relating depth and parent_depth, which is a desired effect. The warning does not show up if the
// skip_child() function is not marked inline).
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING
simdjson_warn_unused simdjson_really_inline error_code json_iterator::skip_child(depth_t parent_depth) noexcept {
  if (depth() <= parent_depth) { return SUCCESS; }

  switch (*advance()) {
    // TODO consider whether matching braces is a requirement: if non-matching braces indicates
    // *missing* braces, then future lookups are not in the object/arrays they think they are,
    // violating the rule "validate enough structure that the user can be confident they are
    // looking at the right values."
    // PERF TODO we can eliminate the switch here with a lookup of how much to add to depth

    // For the first open array/object in a value, we've already incremented depth, so keep it the same
    // We never stop at colon, but if we did, it wouldn't affect depth
    case '[': case '{': case ':':
      logger::log_start_value(*this, "skip");
      break;
    // If there is a comma, we have just finished a value in an array/object, and need to get back in
    case ',':
      logger::log_value(*this, "skip");
      break;
    // ] or } means we just finished a value and need to jump out of the array/object
    case ']': case '}':
      logger::log_end_value(*this, "skip");
      _depth--;
      if (depth() <= parent_depth) { return SUCCESS; }
      break;
    // Anything else must be a scalar value
    default:
      // For the first scalar, we will have incremented depth already, so we decrement it here.
      logger::log_value(*this, "skip");
      _depth--;
      if (depth() <= parent_depth) { return SUCCESS; }
      break;
  }

  // Now that we've considered the first value, we only increment/decrement for arrays/objects
  auto end = &parser->dom_parser->structural_indexes[parser->dom_parser->n_structural_indexes];
  while (token.index <= end) {
    switch (*advance()) {
      case '[': case '{':
        logger::log_start_value(*this, "skip");
        _depth++;
        break;
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      // PERF TODO we can eliminate the switch here with a lookup of how much to add to depth
      case ']': case '}':
        logger::log_end_value(*this, "skip");
        _depth--;
        if (depth() <= parent_depth) { return SUCCESS; }
        break;
      default:
        logger::log_value(*this, "skip", "");
        break;
    }
  }

  return report_error(TAPE_ERROR, "not enough close braces");
}

SIMDJSON_POP_DISABLE_WARNINGS

simdjson_really_inline bool json_iterator::at_root() const noexcept {
  return token.position() == root_checkpoint();
}

simdjson_really_inline token_position json_iterator::root_checkpoint() const noexcept {
  return parser->dom_parser->structural_indexes.get();
}

simdjson_really_inline void json_iterator::assert_at_root() const noexcept {
  SIMDJSON_ASSUME( _depth == 1 );
  // Visual Studio Clang treats unique_ptr.get() as "side effecting."
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  SIMDJSON_ASSUME( token.index == parser->dom_parser->structural_indexes.get() );
#endif
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return token.index == &parser->dom_parser->structural_indexes[parser->dom_parser->n_structural_indexes];
}

simdjson_really_inline bool json_iterator::is_alive() const noexcept {
  return parser;
}

simdjson_really_inline void json_iterator::abandon() noexcept {
  parser = nullptr;
  _depth = 0;
}

simdjson_really_inline const uint8_t *json_iterator::advance() noexcept {
  return token.advance();
}

simdjson_really_inline const uint8_t *json_iterator::peek(int32_t delta) const noexcept {
  return token.peek(delta);
}

simdjson_really_inline uint32_t json_iterator::peek_length(int32_t delta) const noexcept {
  return token.peek_length(delta);
}

simdjson_really_inline const uint8_t *json_iterator::peek(token_position position) const noexcept {
  return token.peek(position);
}

simdjson_really_inline uint32_t json_iterator::peek_length(token_position position) const noexcept {
  return token.peek_length(position);
}

simdjson_really_inline void json_iterator::ascend_to(depth_t parent_depth) noexcept {
  SIMDJSON_ASSUME(parent_depth >= 0 && parent_depth < INT32_MAX - 1);
  SIMDJSON_ASSUME(_depth == parent_depth + 1);
  _depth = parent_depth;
}

simdjson_really_inline void json_iterator::descend_to(depth_t child_depth) noexcept {
  SIMDJSON_ASSUME(child_depth >= 1 && child_depth < INT32_MAX);
  SIMDJSON_ASSUME(_depth == child_depth - 1);
  _depth = child_depth;
}

simdjson_really_inline depth_t json_iterator::depth() const noexcept {
  return _depth;
}

simdjson_really_inline uint8_t *&json_iterator::string_buf_loc() noexcept {
  return _string_buf_loc;
}

simdjson_really_inline error_code json_iterator::report_error(error_code _error, const char *message) noexcept {
  SIMDJSON_ASSUME(_error != SUCCESS && _error != UNINITIALIZED && _error != INCORRECT_TYPE && _error != NO_SUCH_FIELD);
  logger::log_error(*this, message);
  error = _error;
  return error;
}

simdjson_really_inline token_position json_iterator::position() const noexcept {
  return token.position();
}
simdjson_really_inline void json_iterator::set_position(token_position target_checkpoint) noexcept {
  token.set_position(target_checkpoint);
}


simdjson_really_inline error_code json_iterator::optional_error(error_code _error, const char *message) noexcept {
  SIMDJSON_ASSUME(_error == INCORRECT_TYPE || _error == NO_SUCH_FIELD);
  logger::log_error(*this, message);
  return _error;
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::copy_to_buffer(const uint8_t *json, uint32_t max_len, uint8_t (&tmpbuf)[N]) noexcept {
  // Truncate whitespace to fit the buffer.
  if (max_len > N-1) {
    if (jsoncharutils::is_not_structural_or_whitespace(json[N-1])) { return false; }
    max_len = N-1;
  }

  // Copy to the buffer.
  std::memcpy(tmpbuf, json, max_len);
  tmpbuf[max_len] = ' ';
  return true;
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::peek_to_buffer(uint8_t (&tmpbuf)[N]) noexcept {
  auto max_len = token.peek_length();
  auto json = token.peek();
  return copy_to_buffer(json, max_len, tmpbuf);
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::advance_to_buffer(uint8_t (&tmpbuf)[N]) noexcept {
  auto max_len = peek_length();
  auto json = advance();
  return copy_to_buffer(json, max_len, tmpbuf);
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(error) {}

} // namespace simdjson