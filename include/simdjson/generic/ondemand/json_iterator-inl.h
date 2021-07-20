namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline json_iterator::json_iterator(json_iterator &&other) noexcept
  : token(std::forward<token_iterator>(other.token)),
    parser{other.parser},
    _string_buf_loc{other._string_buf_loc},
    error{other.error},
    _depth{other._depth},
    _root{other._root},
    _streaming{other._streaming}
{
  other.parser = nullptr;
}
simdjson_really_inline json_iterator &json_iterator::operator=(json_iterator &&other) noexcept {
  token = other.token;
  parser = other.parser;
  _string_buf_loc = other._string_buf_loc;
  error = other.error;
  _depth = other._depth;
  _root = other._root;
  _streaming = other._streaming;
  other.parser = nullptr;
  return *this;
}

simdjson_really_inline json_iterator::json_iterator(const uint8_t *buf, ondemand::parser *_parser) noexcept
  : token(buf, _parser->implementation->structural_indexes.get()),
    parser{_parser},
    _string_buf_loc{parser->string_buf.get()},
    _depth{1},
    _root{parser->implementation->structural_indexes.get()},
    _streaming{false}

{
  logger::log_headers();
}

inline void json_iterator::rewind() noexcept {
  token.index = _root;
  logger::log_headers(); // We start again
  _string_buf_loc = parser->string_buf.get();
  _depth = 1;
}

// GCC 7 warns when the first line of this function is inlined away into oblivion due to the caller
// relating depth and parent_depth, which is a desired effect. The warning does not show up if the
// skip_child() function is not marked inline).
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING
simdjson_warn_unused simdjson_really_inline error_code json_iterator::skip_child(depth_t parent_depth) noexcept {
  /***
   * WARNING:
   * Inside an object, a string value is a depth of +1 compared to the object. Yet a key
   * is at the same depth as the object.
   * But json_iterator cannot easily tell whether we are pointing at a key or a string value.
   * Instead, it assumes that if you are pointing at a string, then it is a value, not a key.
   * To be clear...
   * the following code assumes that we are *not* pointing at a key. If we are then a bug
   * will follow. Unfortunately, it is not possible for the json_iterator its to make this
   * check.
   */
  if (depth() <= parent_depth) { return SUCCESS; }
  switch (*return_current_and_advance()) {
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
    /*case '"':
      if(*peek() == ':') {
        // we are at a key!!! This is
        // only possible if someone searched
        // for a key in an object and the key
        // was not found but our code then
        // decided the consume the separating
        // comma before returning.
        logger::log_value(*this, "key");
        advance(); // eat up the ':'
        break; // important!!!
      }
      simdjson_fallthrough;*/
    // Anything else must be a scalar value
    default:
      // For the first scalar, we will have incremented depth already, so we decrement it here.
      logger::log_value(*this, "skip");
      _depth--;
      if (depth() <= parent_depth) { return SUCCESS; }
      break;
  }

  // Now that we've considered the first value, we only increment/decrement for arrays/objects
  auto end = &parser->implementation->structural_indexes[parser->implementation->n_structural_indexes];
  while (token.index <= end) {
    switch (*return_current_and_advance()) {
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

simdjson_really_inline bool json_iterator::streaming() const noexcept {
  return _streaming;
}

simdjson_really_inline token_position json_iterator::root_checkpoint() const noexcept {
  return _root;
}

simdjson_really_inline void json_iterator::assert_at_root() const noexcept {
  SIMDJSON_ASSUME( _depth == 1 );
  // Visual Studio Clang treats unique_ptr.get() as "side effecting."
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  SIMDJSON_ASSUME( token.index == _root );
#endif
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return token.index == &parser->implementation->structural_indexes[parser->implementation->n_structural_indexes];
}

inline std::string json_iterator::to_string() const noexcept {
  if( !is_alive() ) { return "dead json_iterator instance"; }
  const char * current_structural = reinterpret_cast<const char *>(token.peek());
  return std::string("json_iterator [ depth : ") + std::to_string(_depth)
          + std::string(", structural : '") + std::string(current_structural,1)
          + std::string("', offset : ") + std::to_string(token.current_offset())
          + std::string("', error : ") + error_message(error)
          + std::string(" ]");
}

simdjson_really_inline bool json_iterator::is_alive() const noexcept {
  return parser;
}

simdjson_really_inline void json_iterator::abandon() noexcept {
  parser = nullptr;
  _depth = 0;
}

simdjson_really_inline const uint8_t *json_iterator::return_current_and_advance() noexcept {
  return token.return_current_and_advance();
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

simdjson_really_inline token_position json_iterator::last_document_position() const noexcept {
  // The following line fails under some compilers...
  // SIMDJSON_ASSUME(parser->implementation->n_structural_indexes > 0);
  // since it has side-effects.
  uint32_t n_structural_indexes{parser->implementation->n_structural_indexes};
  SIMDJSON_ASSUME(n_structural_indexes > 0);
  return &parser->implementation->structural_indexes[n_structural_indexes - 1];
}
simdjson_really_inline const uint8_t *json_iterator::peek_last() const noexcept {
  return token.peek(last_document_position());
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
simdjson_really_inline void json_iterator::reenter_child(token_position position, depth_t child_depth) noexcept {
  SIMDJSON_ASSUME(child_depth >= 1 && child_depth < INT32_MAX);
  SIMDJSON_ASSUME(_depth == child_depth - 1);
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  SIMDJSON_ASSUME(position >= parser->start_positions[child_depth]);
#endif
#endif
  token.set_position(position);
  _depth = child_depth;
}

#ifdef SIMDJSON_DEVELOPMENT_CHECKS
simdjson_really_inline token_position json_iterator::start_position(depth_t depth) const noexcept {
  return parser->start_positions[depth];
}
simdjson_really_inline void json_iterator::set_start_position(depth_t depth, token_position position) noexcept {
  parser->start_positions[depth] = position;
}

#endif


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
  auto json = return_current_and_advance();
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