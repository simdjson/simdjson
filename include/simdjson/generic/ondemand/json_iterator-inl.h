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

simdjson_really_inline json_iterator::json_iterator(ondemand::parser *_parser) noexcept
  : token(_parser->dom_parser.buf, _parser->dom_parser.structural_indexes.get()),
    parser{_parser},
    _string_buf_loc{parser->string_buf.get()},
    _depth{1}
{
  // Release the string buf so it can be reused by the next document
  logger::log_headers();
}

simdjson_warn_unused simdjson_really_inline error_code json_iterator::skip_child(depth_t parent_depth) noexcept {
  SIMDJSON_ASSUME(depth() == parent_depth+1);

  switch (*advance()) {
    case '[': case '{':
      logger::log_start_value(*this, "skip");
      return finish_child(parent_depth);
    default:
      logger::log_value(*this, "skip");
      ascend_to(parent_depth);
      return SUCCESS;
  }
}

simdjson_warn_unused simdjson_really_inline error_code json_iterator::finish_child(depth_t parent_depth) noexcept {
  if (depth() <= parent_depth) { return SUCCESS; }
  // The loop breaks only when depth()-- happens.
  auto end = &parser->dom_parser.structural_indexes[parser->dom_parser.n_structural_indexes];
  while (token.index <= end) {
    uint8_t ch = *advance();
    switch (ch) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}':
        logger::log_end_value(*this, "skip");
        _depth--;
        if (depth() <= parent_depth) { return SUCCESS; }
        break;
      // PERF TODO does it skip the depth() check when we don't decrement depth()?
      case '[': case '{':
        logger::log_start_value(*this, "skip");
        _depth++;
        break;
      default:
        logger::log_value(*this, "skip", "");
        break;
    }
  }

  return report_error(TAPE_ERROR, "not enough close braces");
}

simdjson_really_inline bool json_iterator::at_start() const noexcept {
  return token.index == parser->dom_parser.structural_indexes.get();
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return token.index == &parser->dom_parser.structural_indexes[parser->dom_parser.n_structural_indexes];
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

simdjson_really_inline void json_iterator::ascend_to(depth_t parent_depth) noexcept {
  SIMDJSON_ASSUME(depth() == parent_depth + 1);
  _depth = parent_depth;
}

simdjson_really_inline void json_iterator::descend_to(depth_t child_depth) noexcept {
  SIMDJSON_ASSUME(depth() == child_depth - 1);
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

simdjson_really_inline error_code json_iterator::optional_error(error_code _error, const char *message) noexcept {
  SIMDJSON_ASSUME(_error == INCORRECT_TYPE || _error == NO_SUCH_FIELD);
  logger::log_error(*this, message);
  return _error;
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::copy_to_buffer(const uint8_t *json, uint32_t max_len, uint8_t (&tmpbuf)[N]) noexcept {
  // Truncate whitespace to fit the buffer.
  if (max_len > N-1) {
    if (jsoncharutils::is_not_structural_or_whitespace(json[N])) { return false; }
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