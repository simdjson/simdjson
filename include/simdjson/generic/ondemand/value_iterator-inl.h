namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline value_iterator::value_iterator(json_iterator *json_iter, depth_t depth, const uint32_t *start_index) noexcept
  : _json_iter{json_iter},
    _depth{depth},
    _start_index{start_index}
{
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::start_object() noexcept {
  assert_at_start();

  if (*_json_iter->advance() != '{') { logger::log_error(*_json_iter, "Not an object"); return INCORRECT_TYPE; }
  return started_object();
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::try_start_object() noexcept {
  assert_at_start();

  if (*_json_iter->peek() != '{') { logger::log_error(*_json_iter, "Not an object"); return INCORRECT_TYPE; }
  _json_iter->advance();
  return started_object();
}

simdjson_warn_unused simdjson_really_inline bool value_iterator::started_object() noexcept {
  if (*_json_iter->peek() == '}') {
    logger::log_value(*_json_iter, "empty object");
    _json_iter->advance();
    _json_iter->ascend_to(depth()-1);
    return false;
  }
  _json_iter->descend_to(depth()+1);
  logger::log_start_value(*_json_iter, "object");
  return true;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::has_next_field() noexcept {
  assert_at_next();

  switch (*_json_iter->advance()) {
    case '}':
      logger::log_end_value(*_json_iter, "object");
      _json_iter->ascend_to(depth()-1);
      return false;
    case ',':
      _json_iter->descend_to(depth()+1);
      return true;
    default:
      return _json_iter->report_error(TAPE_ERROR, "Missing comma between object fields");
  }
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::find_field_raw(const std::string_view key) noexcept {
  error_code error;
  bool has_value;

  //
  // Initially, the object can be in one of a few different places:
  //
  // 1. The start of the object, at the first field:
  //
  //    ```
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //      ^ (depth 2, index 1)
  //    ```
  //
  // 2. When a previous search did not yield a value or the object is empty:
  //
  //    ```
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                                     ^ (depth 0)
  //    { }
  //        ^ (depth 0, index 2)
  //    ```
  //
  if (!is_open()) { return false; }
  if (at_first_field()) {
    has_value = true;

  // 3. When a previous search found a field or an iterator yielded a value:
  //
  //    ```
  //    // When a field was not fully consumed (or not even touched at all)
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //           ^ (depth 2)
  //    // When a field was fully consumed
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                   ^ (depth 1)
  //    // When the last field was fully consumed
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                                   ^ (depth 1)
  //    ```
  //
  } else {
    if ((error = skip_child() )) { abandon(); return error; }
    if ((error = has_next_field().get(has_value) )) { abandon(); return error; }
  }
  while (has_value) {
    // Get the key and colon, stopping at the value.
    raw_json_string actual_key;
    if ((error = field_key().get(actual_key) )) { abandon(); return error; };
    if ((error = field_value() )) { abandon(); return error; }

    // If it matches, stop and return
    if (actual_key == key) {
      logger::log_event(*this, "match", key, -2);
      return true;
    }

    // No match: skip the value and see if , or } is next
    logger::log_event(*this, "no match", key, -2);
    SIMDJSON_TRY( skip_child() ); // Skip the value entirely
    if ((error = has_next_field().get(has_value) )) { abandon(); return error; }
  }

  // If the loop ended, we're out of fields to look at.
  return false;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::find_field_unordered_raw(const std::string_view key) noexcept {
  error_code error;
  bool has_value;

  //
  // Initially, the object can be in one of a few different places:
  //
  // 1. The start of the object, at the first field:
  //
  //    ```
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //      ^ (depth 2, index 1)
  //    ```
  //
  if (at_first_field()) {
    // If we're at the beginning of the object, we definitely have a field
    has_value = true;

  // 2. When a previous search did not yield a value or the object is empty:
  //
  //    ```
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                                     ^ (depth 0)
  //    { }
  //        ^ (depth 0, index 2)
  //    ```
  //
  } else if (!is_open()) {
    has_value = false;

  // 3. When a previous search found a field or an iterator yielded a value:
  //
  //    ```
  //    // When a field was not fully consumed (or not even touched at all)
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //           ^ (depth 2)
  //    // When a field was fully consumed
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                   ^ (depth 1)
  //    // When the last field was fully consumed
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                                   ^ (depth 1)
  //    ```
  //
  } else {
    // Finish the previous value and see if , or } is next
    if ((error = skip_child() )) { abandon(); return error; }
    if ((error = has_next_field().get(has_value) )) { abandon(); return error; }
  }

  // After initial processing, we will be in one of two states:
  //
  // ```
  // // At the beginning of a field
  // { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //   ^ (depth 1)
  // { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                  ^ (depth 1)
  // // At the end of the object
  // { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                                  ^ (depth 0)
  // ```
  //

  // First, we scan from that point to the end.
  // If we don't find a match, we loop back around, and scan from the beginning to that point.
  const uint32_t *search_start = _json_iter->checkpoint();

  // Next, we find a match starting from the current position.
  while (has_value) {
    SIMDJSON_ASSUME( _json_iter->_depth == _depth + 1 ); // We must be at the start of a field

    // Get the key and colon, stopping at the value.
    raw_json_string actual_key;
    if ((error = field_key().get(actual_key) )) { abandon(); return error; };
    if ((error = field_value() )) { abandon(); return error; }

    // If it matches, stop and return
    if (actual_key == key) {
      logger::log_event(*this, "match", key, -2);
      return true;
    }

    // No match: skip the value and see if , or } is next
    logger::log_event(*this, "no match", key, -2);
    SIMDJSON_TRY( skip_child() );
    if ((error = has_next_field().get(has_value) )) { abandon(); return error; }
  }

  // If we reach the end without finding a match, search the rest of the fields starting at the
  // beginning of the object.
  // (We have already run through the object before, so we've already validated its structure. We
  // don't check errors in this bit.)
  _json_iter->restore_checkpoint(_start_index + 1);
  _json_iter->descend_to(_depth);

  has_value = started_object();
  while (_json_iter->checkpoint() < search_start) {
    SIMDJSON_ASSUME(has_value); // we should reach search_start before ever reaching the end of the object
    SIMDJSON_ASSUME( _json_iter->_depth == _depth + 1 ); // We must be at the start of a field

    // Get the key and colon, stopping at the value.
    raw_json_string actual_key;
    error = field_key().get(actual_key); SIMDJSON_ASSUME(!error);
    error = field_value(); SIMDJSON_ASSUME(!error);

    // If it matches, stop and return
    if (actual_key == key) {
      logger::log_event(*this, "match", key, -2);
      return true;
    }

    // No match: skip the value and see if , or } is next
    logger::log_event(*this, "no match", key, -2);
    SIMDJSON_TRY( skip_child() );
    error = has_next_field().get(has_value); SIMDJSON_ASSUME(!error);
  }

  // If the loop ended, we're out of fields to look at.
  return false;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> value_iterator::field_key() noexcept {
  assert_at_child();

  const uint8_t *key = _json_iter->advance();
  if (*(key++) != '"') { return _json_iter->report_error(TAPE_ERROR, "Object key is not a string"); }
  return raw_json_string(key);
}

simdjson_warn_unused simdjson_really_inline error_code value_iterator::field_value() noexcept {
  assert_at_child();

  if (*_json_iter->advance() != ':') { return _json_iter->report_error(TAPE_ERROR, "Missing colon in object field"); }
  return SUCCESS;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::start_array() noexcept {
  assert_at_start();

  if (*_json_iter->advance() != '[') { logger::log_error(*_json_iter, "Not an array"); return INCORRECT_TYPE; }
  return started_array();
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::try_start_array() noexcept {
  assert_at_start();

  if (*_json_iter->peek() != '[') { logger::log_error(*_json_iter, "Not an array"); return INCORRECT_TYPE; }
  _json_iter->advance();
  return started_array();
}

simdjson_warn_unused simdjson_really_inline bool value_iterator::started_array() noexcept {
  if (*_json_iter->peek() == ']') {
    logger::log_value(*_json_iter, "empty array");
    _json_iter->advance();
    _json_iter->ascend_to(depth()-1);
    return false;
  }
  logger::log_start_value(*_json_iter, "array");
  _json_iter->descend_to(depth()+1);
  return true;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::has_next_element() noexcept {
  assert_at_next();

  switch (*_json_iter->advance()) {
    case ']':
      logger::log_end_value(*_json_iter, "array");
      _json_iter->ascend_to(depth()-1);
      return false;
    case ',':
      _json_iter->descend_to(depth()+1);
      return true;
    default:
      return _json_iter->report_error(TAPE_ERROR, "Missing comma between array elements");
  }
}

simdjson_warn_unused simdjson_really_inline simdjson_result<std::string_view> value_iterator::try_get_string() noexcept {
  return try_get_raw_json_string().unescape(_json_iter->string_buf_loc());
}
simdjson_warn_unused simdjson_really_inline simdjson_result<std::string_view> value_iterator::require_string() noexcept {
  return require_raw_json_string().unescape(_json_iter->string_buf_loc());
}
simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> value_iterator::try_get_raw_json_string() noexcept {
  assert_at_start();

  logger::log_value(*_json_iter, "string", "", 0);
  auto json = _json_iter->peek();
  if (*json != '"') { logger::log_error(*_json_iter, "Not a string"); return INCORRECT_TYPE; }
  _json_iter->advance();
  _json_iter->ascend_to(depth()-1);
  return raw_json_string(json+1);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> value_iterator::require_raw_json_string() noexcept {
  assert_at_start();

  logger::log_value(*_json_iter, "string", "", 0);
  auto json = _json_iter->advance();
  if (*json != '"') { logger::log_error(*_json_iter, "Not a string"); return INCORRECT_TYPE; }
  _json_iter->ascend_to(depth()-1);
  return raw_json_string(json+1);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::try_get_uint64() noexcept {
  assert_at_non_root_start();

  logger::log_value(*_json_iter, "uint64", "", 0);
  uint64_t result;
  SIMDJSON_TRY( numberparsing::parse_unsigned(_json_iter->peek()).get(result) );
  _json_iter->advance();
  _json_iter->ascend_to(depth()-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::require_uint64() noexcept {
  assert_at_non_root_start();

  logger::log_value(*_json_iter, "uint64", "", 0);
  _json_iter->ascend_to(depth()-1);
  return numberparsing::parse_unsigned(_json_iter->advance());
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::try_get_int64() noexcept {
  assert_at_non_root_start();

  logger::log_value(*_json_iter, "int64", "", 0);
  int64_t result;
  SIMDJSON_TRY( numberparsing::parse_integer(_json_iter->peek()).get(result) );
  _json_iter->advance();
  _json_iter->ascend_to(depth()-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::require_int64() noexcept {
  assert_at_non_root_start();

  logger::log_value(*_json_iter, "int64", "", 0);
  _json_iter->ascend_to(depth()-1);
  return numberparsing::parse_integer(_json_iter->advance());
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::try_get_double() noexcept {
  assert_at_non_root_start();

  logger::log_value(*_json_iter, "double", "", 0);
  double result;
  SIMDJSON_TRY( numberparsing::parse_double(_json_iter->peek()).get(result) );
  _json_iter->advance();
  _json_iter->ascend_to(depth()-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::require_double() noexcept {
  assert_at_non_root_start();

  logger::log_value(*_json_iter, "double", "", 0);
  _json_iter->ascend_to(depth()-1);
  return numberparsing::parse_double(_json_iter->advance());
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::parse_bool(const uint8_t *json) const noexcept {
  logger::log_value(*_json_iter, "bool", "");
  auto not_true = atomparsing::str4ncmp(json, "true");
  auto not_false = atomparsing::str4ncmp(json, "fals") | (json[4] ^ 'e');
  bool error = (not_true && not_false) || jsoncharutils::is_not_structural_or_whitespace(json[not_true ? 5 : 4]);
  if (error) { logger::log_error(*_json_iter, "Not a boolean"); return INCORRECT_TYPE; }
  return simdjson_result<bool>(!not_true);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::try_get_bool() noexcept {
  assert_at_non_root_start();

  bool result;
  SIMDJSON_TRY( parse_bool(_json_iter->peek()).get(result) );
  _json_iter->advance();
  _json_iter->ascend_to(depth()-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::require_bool() noexcept {
  assert_at_non_root_start();

  _json_iter->ascend_to(depth()-1);
  return parse_bool(_json_iter->advance());
}
simdjson_really_inline bool value_iterator::is_null(const uint8_t *json) const noexcept {
  if (!atomparsing::str4ncmp(json, "null")) {
    logger::log_value(*_json_iter, "null", "");
    return true;
  }
  return false;
}
simdjson_really_inline bool value_iterator::is_null() noexcept {
  assert_at_non_root_start();

  if (is_null(_json_iter->peek())) {
    _json_iter->advance();
    _json_iter->ascend_to(depth()-1);
    return true;
  }
  return false;
}
simdjson_really_inline bool value_iterator::require_null() noexcept {
  assert_at_non_root_start();

  _json_iter->ascend_to(depth()-1);
  return is_null(_json_iter->advance());
}

constexpr const uint32_t MAX_INT_LENGTH = 1024;

simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::parse_root_uint64(const uint8_t *json, uint32_t max_len) const noexcept {
  uint8_t tmpbuf[20+1]; // <20 digits> is the longest possible unsigned integer
  if (!_json_iter->copy_to_buffer(json, max_len, tmpbuf)) { logger::log_error(*_json_iter, "Root number more than 20 characters"); return NUMBER_ERROR; }
  logger::log_value(*_json_iter, "uint64", "", 0);
  auto result = numberparsing::parse_unsigned(tmpbuf);
  if (result.error()) { logger::log_error(*_json_iter, "Error parsing unsigned integer"); }
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::try_get_root_uint64() noexcept {
  assert_at_root();

  uint64_t result;
  SIMDJSON_TRY( parse_root_uint64(_json_iter->peek(), _json_iter->peek_length()).get(result) );
  _json_iter->advance();
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::require_root_uint64() noexcept {
  assert_at_root();

  auto max_len = _json_iter->peek_length();
  return parse_root_uint64(_json_iter->advance(), max_len);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::parse_root_int64(const uint8_t *json, uint32_t max_len) const noexcept {
  uint8_t tmpbuf[20+1]; // -<19 digits> is the longest possible integer
  if (!_json_iter->copy_to_buffer(json, max_len, tmpbuf)) { logger::log_error(*_json_iter, "Root number more than 20 characters"); return NUMBER_ERROR; }
  logger::log_value(*_json_iter, "int64", "", 0);
  auto result = numberparsing::parse_integer(tmpbuf);
  if (result.error()) { logger::log_error(*_json_iter, "Error parsing integer"); }
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::try_get_root_int64() noexcept {
  assert_at_root();

  int64_t result;
  SIMDJSON_TRY( parse_root_int64(_json_iter->peek(), _json_iter->peek_length()).get(result) );
  _json_iter->advance();
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::require_root_int64() noexcept {
  assert_at_root();

  auto max_len = _json_iter->peek_length();
  return parse_root_int64(_json_iter->advance(), max_len);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::parse_root_double(const uint8_t *json, uint32_t max_len) const noexcept {
  // Per https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/, 1074 is the maximum number of significant fractional digits. Add 8 more digits for the biggest number: -0.<fraction>e-308.
  uint8_t tmpbuf[1074+8+1];
  if (!_json_iter->copy_to_buffer(json, max_len, tmpbuf)) { logger::log_error(*_json_iter, "Root number more than 1082 characters"); return NUMBER_ERROR; }
  logger::log_value(*_json_iter, "double", "", 0);
  auto result = numberparsing::parse_double(tmpbuf);
  if (result.error()) { logger::log_error(*_json_iter, "Error parsing double"); }
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::try_get_root_double() noexcept {
  assert_at_root();

  double result;
  SIMDJSON_TRY( parse_root_double(_json_iter->peek(), _json_iter->peek_length()).get(result) );
  _json_iter->advance();
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::require_root_double() noexcept {
  assert_at_root();

  auto max_len = _json_iter->peek_length();
  return parse_root_double(_json_iter->advance(), max_len);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::parse_root_bool(const uint8_t *json, uint32_t max_len) const noexcept {
  uint8_t tmpbuf[5+1];
  if (!_json_iter->copy_to_buffer(json, max_len, tmpbuf)) { logger::log_error(*_json_iter, "Not a boolean"); return INCORRECT_TYPE; }
  return parse_bool(tmpbuf);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::try_get_root_bool() noexcept {
  assert_at_root();

  bool result;
  SIMDJSON_TRY( parse_root_bool(_json_iter->peek(), _json_iter->peek_length()).get(result) );
  _json_iter->advance();
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::require_root_bool() noexcept {
  assert_at_root();

  auto max_len = _json_iter->peek_length();
  return parse_root_bool(_json_iter->advance(), max_len);
}
simdjson_really_inline bool value_iterator::is_root_null(const uint8_t *json, uint32_t max_len) const noexcept {
  uint8_t tmpbuf[4+1];
  if (!_json_iter->copy_to_buffer(json, max_len, tmpbuf)) { return false; }
  return is_null(tmpbuf);
}
simdjson_really_inline bool value_iterator::is_root_null() noexcept {
  assert_at_root();

  if (!is_root_null(_json_iter->peek(), _json_iter->peek_length())) { return false; }
  _json_iter->advance();
  return true;
}
simdjson_really_inline bool value_iterator::require_root_null() noexcept {
  assert_at_root();

  auto max_len = _json_iter->peek_length();
  return is_root_null(_json_iter->advance(), max_len);
}

simdjson_warn_unused simdjson_really_inline error_code value_iterator::skip_child() noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index > _start_index );
  SIMDJSON_ASSUME( _json_iter->_depth >= _depth );

  return _json_iter->skip_child(depth());
}
simdjson_really_inline value_iterator value_iterator::child() const noexcept {
  assert_at_child();
  return { _json_iter, depth()+1, _json_iter->token.checkpoint() };
}

simdjson_really_inline bool value_iterator::is_open() const noexcept {
  return _json_iter->depth() >= depth();
}

simdjson_really_inline bool value_iterator::at_eof() const noexcept {
  return _json_iter->at_eof();
}

simdjson_really_inline bool value_iterator::at_start() const noexcept {
  return _json_iter->token.index == _start_index;
}

simdjson_really_inline bool value_iterator::at_first_field() const noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index > _start_index );
  return _json_iter->token.index == _start_index + 1;
}

simdjson_really_inline void value_iterator::abandon() noexcept {
  _json_iter->abandon();
}


simdjson_warn_unused simdjson_really_inline depth_t value_iterator::depth() const noexcept {
  return _depth;
}
simdjson_warn_unused simdjson_really_inline error_code value_iterator::error() const noexcept {
  return _json_iter->error;
}
simdjson_warn_unused simdjson_really_inline uint8_t *&value_iterator::string_buf_loc() noexcept {
  return _json_iter->string_buf_loc();
}
simdjson_warn_unused simdjson_really_inline const json_iterator &value_iterator::json_iter() const noexcept {
  return *_json_iter;
}
simdjson_warn_unused simdjson_really_inline json_iterator &value_iterator::json_iter() noexcept {
  return *_json_iter;
}

simdjson_really_inline void value_iterator::assert_at_start() const noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index == _start_index );
  SIMDJSON_ASSUME( _json_iter->_depth == _depth );
  SIMDJSON_ASSUME( _depth > 0 );
}

simdjson_really_inline void value_iterator::assert_at_next() const noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index > _start_index );
  SIMDJSON_ASSUME( _json_iter->_depth == _depth );
  SIMDJSON_ASSUME( _depth > 0 );
}

simdjson_really_inline void value_iterator::assert_at_child() const noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index > _start_index );
  SIMDJSON_ASSUME( _json_iter->_depth == _depth + 1 );
  SIMDJSON_ASSUME( _depth > 0 );
}

simdjson_really_inline void value_iterator::assert_at_root() const noexcept {
  assert_at_start();
  SIMDJSON_ASSUME( _depth == 1 );
}

simdjson_really_inline void value_iterator::assert_at_non_root_start() const noexcept {
  assert_at_start();
  SIMDJSON_ASSUME( _depth > 1 );
}


} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::value_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator>(error) {}

} // namespace simdjson