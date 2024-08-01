#ifndef SIMDJSON_GENERIC_ONDEMAND_JSON_ITERATOR_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_JSON_ITERATOR_INL_H
#include "simdjson/internal/dom_parser_implementation.h"
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/json_iterator.h"
#include "simdjson/generic/ondemand/parser.h"
#include "simdjson/generic/ondemand/raw_json_string.h"
#include "simdjson/generic/ondemand/logger-inl.h"
#include "simdjson/generic/ondemand/parser-inl.h"
#include "simdjson/generic/ondemand/token_iterator-inl.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_inline json_iterator::json_iterator(json_iterator &&other) noexcept
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
simdjson_inline json_iterator &json_iterator::operator=(json_iterator &&other) noexcept {
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

simdjson_inline json_iterator::json_iterator(const uint8_t *buf, ondemand::parser *_parser) noexcept
  : token(buf, &_parser->implementation->structural_indexes[0]),
    parser{_parser},
    _string_buf_loc{parser->string_buf.get()},
    _depth{1},
    _root{parser->implementation->structural_indexes.get()},
    _streaming{false}

{
  logger::log_headers();
#if SIMDJSON_CHECK_EOF
  assert_more_tokens();
#endif
}

#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
simdjson_inline json_iterator::json_iterator(const uint8_t *buf, ondemand::parser *_parser, bool streaming) noexcept
    : token(buf, &_parser->implementation->structural_indexes[0]),
      parser{_parser},
      _string_buf_loc{parser->string_buf.get()},
      _depth{1},
      _root{parser->implementation->structural_indexes.get()},
      _streaming{streaming}

{
  logger::log_headers();
#if SIMDJSON_CHECK_EOF
  assert_more_tokens();
#endif
}
#endif // SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON

inline void json_iterator::rewind() noexcept {
  token.set_position( root_position() );
  logger::log_headers(); // We start again
  _string_buf_loc = parser->string_buf.get();
  _depth = 1;
}

inline bool json_iterator::balanced() const noexcept {
  token_iterator ti(token);
  int32_t count{0};
  ti.set_position( root_position() );
  while(ti.peek() <= peek_last()) {
    switch (*ti.return_current_and_advance())
    {
    case '[': case '{':
      count++;
      break;
    case ']': case '}':
      count--;
      break;
    default:
      break;
    }
  }
  return count == 0;
}


// GCC 7 warns when the first line of this function is inlined away into oblivion due to the caller
// relating depth and parent_depth, which is a desired effect. The warning does not show up if the
// skip_child() function is not marked inline).
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING
simdjson_warn_unused simdjson_inline error_code json_iterator::skip_child(depth_t parent_depth) noexcept {
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
#if SIMDJSON_CHECK_EOF
      // If there are no more tokens, the parent is incomplete.
      if (at_end()) { return report_error(INCOMPLETE_ARRAY_OR_OBJECT, "Missing [ or { at start"); }
#endif // SIMDJSON_CHECK_EOF
      break;
    case '"':
      if(*peek() == ':') {
        // We are at a key!!!
        // This might happen if you just started an object and you skip it immediately.
        // Performance note: it would be nice to get rid of this check as it is somewhat
        // expensive.
        // https://github.com/simdjson/simdjson/issues/1742
        logger::log_value(*this, "key");
        return_current_and_advance(); // eat up the ':'
        break; // important!!!
      }
      simdjson_fallthrough;
    // Anything else must be a scalar value
    default:
      // For the first scalar, we will have incremented depth already, so we decrement it here.
      logger::log_value(*this, "skip");
      _depth--;
      if (depth() <= parent_depth) { return SUCCESS; }
      break;
  }

  // Now that we've considered the first value, we only increment/decrement for arrays/objects
  while (position() < end_position()) {
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

simdjson_inline bool json_iterator::at_root() const noexcept {
  return position() == root_position();
}

simdjson_inline bool json_iterator::is_single_token() const noexcept {
  return parser->implementation->n_structural_indexes == 1;
}

simdjson_inline bool json_iterator::streaming() const noexcept {
  return _streaming;
}

simdjson_inline token_position json_iterator::root_position() const noexcept {
  return _root;
}

simdjson_inline void json_iterator::assert_at_document_depth() const noexcept {
  SIMDJSON_ASSUME( _depth == 1 );
}

simdjson_inline void json_iterator::assert_at_root() const noexcept {
  SIMDJSON_ASSUME( _depth == 1 );
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  // Under Visual Studio, the next SIMDJSON_ASSUME fails with: the argument
  // has side effects that will be discarded.
  SIMDJSON_ASSUME( token.position() == _root );
#endif
}

simdjson_inline void json_iterator::assert_more_tokens(uint32_t required_tokens) const noexcept {
  assert_valid_position(token._position + required_tokens - 1);
}

simdjson_inline void json_iterator::assert_valid_position(token_position position) const noexcept {
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  SIMDJSON_ASSUME( position >= &parser->implementation->structural_indexes[0] );
  SIMDJSON_ASSUME( position < &parser->implementation->structural_indexes[parser->implementation->n_structural_indexes] );
#endif
}

simdjson_inline bool json_iterator::at_end() const noexcept {
  return position() == end_position();
}
simdjson_inline token_position json_iterator::end_position() const noexcept {
  uint32_t n_structural_indexes{parser->implementation->n_structural_indexes};
  return &parser->implementation->structural_indexes[n_structural_indexes];
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

inline simdjson_result<const char *> json_iterator::current_location() const noexcept {
  if (!is_alive()) {    // Unrecoverable error
    if (!at_root()) {
      return reinterpret_cast<const char *>(token.peek(-1));
    } else {
      return reinterpret_cast<const char *>(token.peek());
    }
  }
  if (at_end()) {
    return OUT_OF_BOUNDS;
  }
  return reinterpret_cast<const char *>(token.peek());
}

simdjson_inline bool json_iterator::is_alive() const noexcept {
  return parser;
}

simdjson_inline void json_iterator::abandon() noexcept {
  parser = nullptr;
  _depth = 0;
}

simdjson_inline const uint8_t *json_iterator::return_current_and_advance() noexcept {
#if SIMDJSON_CHECK_EOF
  assert_more_tokens();
#endif // SIMDJSON_CHECK_EOF
  return token.return_current_and_advance();
}

simdjson_inline const uint8_t *json_iterator::unsafe_pointer() const noexcept {
  // deliberately done without safety guard:
  return token.peek();
}

simdjson_inline const uint8_t *json_iterator::peek(int32_t delta) const noexcept {
#if SIMDJSON_CHECK_EOF
  assert_more_tokens(delta+1);
#endif // SIMDJSON_CHECK_EOF
  return token.peek(delta);
}

simdjson_inline uint32_t json_iterator::peek_length(int32_t delta) const noexcept {
#if SIMDJSON_CHECK_EOF
  assert_more_tokens(delta+1);
#endif // #if SIMDJSON_CHECK_EOF
  return token.peek_length(delta);
}

simdjson_inline const uint8_t *json_iterator::peek(token_position position) const noexcept {
  // todo: currently we require end-of-string buffering, but the following
  // assert_valid_position should be turned on if/when we lift that condition.
  // assert_valid_position(position);
  // This is almost surely related to SIMDJSON_CHECK_EOF but given that SIMDJSON_CHECK_EOF
  // is ON by default, we have no choice but to disable it for real with a comment.
  return token.peek(position);
}

simdjson_inline uint32_t json_iterator::peek_length(token_position position) const noexcept {
#if SIMDJSON_CHECK_EOF
  assert_valid_position(position);
#endif // SIMDJSON_CHECK_EOF
  return token.peek_length(position);
}
simdjson_inline uint32_t json_iterator::peek_root_length(token_position position) const noexcept {
#if SIMDJSON_CHECK_EOF
  assert_valid_position(position);
#endif // SIMDJSON_CHECK_EOF
  return token.peek_root_length(position);
}

simdjson_inline token_position json_iterator::last_position() const noexcept {
  // The following line fails under some compilers...
  // SIMDJSON_ASSUME(parser->implementation->n_structural_indexes > 0);
  // since it has side-effects.
  uint32_t n_structural_indexes{parser->implementation->n_structural_indexes};
  SIMDJSON_ASSUME(n_structural_indexes > 0);
  return &parser->implementation->structural_indexes[n_structural_indexes - 1];
}
simdjson_inline const uint8_t *json_iterator::peek_last() const noexcept {
  return token.peek(last_position());
}

simdjson_inline void json_iterator::ascend_to(depth_t parent_depth) noexcept {
  SIMDJSON_ASSUME(parent_depth >= 0 && parent_depth < INT32_MAX - 1);
  SIMDJSON_ASSUME(_depth == parent_depth + 1);
  _depth = parent_depth;
}

simdjson_inline void json_iterator::descend_to(depth_t child_depth) noexcept {
  SIMDJSON_ASSUME(child_depth >= 1 && child_depth < INT32_MAX);
  SIMDJSON_ASSUME(_depth == child_depth - 1);
  _depth = child_depth;
}

simdjson_inline depth_t json_iterator::depth() const noexcept {
  return _depth;
}

simdjson_inline uint8_t *&json_iterator::string_buf_loc() noexcept {
  return _string_buf_loc;
}

simdjson_inline error_code json_iterator::report_error(error_code _error, const char *message) noexcept {
  SIMDJSON_ASSUME(_error != SUCCESS && _error != UNINITIALIZED && _error != INCORRECT_TYPE && _error != NO_SUCH_FIELD);
  logger::log_error(*this, message);
  error = _error;
  return error;
}

simdjson_inline token_position json_iterator::position() const noexcept {
  return token.position();
}

simdjson_inline simdjson_result<std::string_view> json_iterator::unescape(raw_json_string in, bool allow_replacement) noexcept {
#if SIMDJSON_DEVELOPMENT_CHECKS
  auto result = parser->unescape(in, _string_buf_loc, allow_replacement);
  SIMDJSON_ASSUME(!parser->string_buffer_overflow(_string_buf_loc));
  return result;
#else
  return parser->unescape(in, _string_buf_loc, allow_replacement);
#endif
}

simdjson_inline simdjson_result<std::string_view> json_iterator::unescape_wobbly(raw_json_string in) noexcept {
#if SIMDJSON_DEVELOPMENT_CHECKS
  auto result = parser->unescape_wobbly(in, _string_buf_loc);
  SIMDJSON_ASSUME(!parser->string_buffer_overflow(_string_buf_loc));
  return result;
#else
  return parser->unescape_wobbly(in, _string_buf_loc);
#endif
}

simdjson_inline void json_iterator::reenter_child(token_position position, depth_t child_depth) noexcept {
  SIMDJSON_ASSUME(child_depth >= 1 && child_depth < INT32_MAX);
  SIMDJSON_ASSUME(_depth == child_depth - 1);
#if SIMDJSON_DEVELOPMENT_CHECKS
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  SIMDJSON_ASSUME(size_t(child_depth) < parser->max_depth());
  SIMDJSON_ASSUME(position >= parser->start_positions[child_depth]);
#endif
#endif
  token.set_position(position);
  _depth = child_depth;
}

simdjson_inline error_code json_iterator::consume_character(char c) noexcept {
  if (*peek() == c) {
    return_current_and_advance();
    return SUCCESS;
  }
  return TAPE_ERROR;
}

#if SIMDJSON_DEVELOPMENT_CHECKS

simdjson_inline token_position json_iterator::start_position(depth_t depth) const noexcept {
  SIMDJSON_ASSUME(size_t(depth) < parser->max_depth());
  return size_t(depth) < parser->max_depth() ? parser->start_positions[depth] : 0;
}

simdjson_inline void json_iterator::set_start_position(depth_t depth, token_position position) noexcept {
  SIMDJSON_ASSUME(size_t(depth) < parser->max_depth());
  if(size_t(depth) < parser->max_depth()) { parser->start_positions[depth] = position; }
}

#endif


simdjson_inline error_code json_iterator::optional_error(error_code _error, const char *message) noexcept {
  SIMDJSON_ASSUME(_error == INCORRECT_TYPE || _error == NO_SUCH_FIELD);
  logger::log_error(*this, message);
  return _error;
}


simdjson_warn_unused simdjson_inline bool json_iterator::copy_to_buffer(const uint8_t *json, uint32_t max_len, uint8_t *tmpbuf, size_t N) noexcept {
  // This function is not expected to be called in performance-sensitive settings.
  // Let us guard against silly cases:
  if((N < max_len) || (N == 0)) { return false; }
  // Copy to the buffer.
  std::memcpy(tmpbuf, json, max_len);
  if(N > max_len) { // We pad whatever remains with ' '.
    std::memset(tmpbuf + max_len, ' ', N - max_len);
  }
  return true;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(value)) {}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(error) {}

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_JSON_ITERATOR_INL_H