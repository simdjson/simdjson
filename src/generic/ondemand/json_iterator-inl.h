namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline json_iterator::json_iterator() noexcept = default;
simdjson_really_inline json_iterator::json_iterator(json_iterator &&other) noexcept = default;
simdjson_really_inline json_iterator &json_iterator::operator=(json_iterator &&other) noexcept = default;
simdjson_really_inline json_iterator::json_iterator(const uint8_t *_buf, uint32_t *_index) noexcept
  : token_iterator(_buf, _index)
{
}


SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::start_object() noexcept {
  if (*advance() != '{') { logger::log_error(*this, "Not an object"); return INCORRECT_TYPE; }
  return started_object();
}

SIMDJSON_WARN_UNUSED simdjson_really_inline bool json_iterator::started_object() noexcept {
  if (*peek() == '}') {
    logger::log_value(*this, "empty object");
    advance();
    return false;
  }
  logger::log_start_value(*this, "object");
  return true;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::has_next_field() noexcept {
  switch (*advance()) {
    case '}':
      logger::log_end_value(*this, "object");
      return false;
    case ',':
      return true;
    default:
      logger::log_error(*this, "Missing comma between object fields");
      return TAPE_ERROR;
  }
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::find_field_raw(const char *key) noexcept {
  bool has_next;
  do {
    raw_json_string actual_key;
    SIMDJSON_TRY( get_raw_json_string().get(actual_key) );
    if (*advance() != ':') { logger::log_error(*this, "Missing colon in object field"); return TAPE_ERROR; }
    if (actual_key == key) {
      logger::log_event(*this, "match", key);
      return true;
    }
    logger::log_event(*this, "non-match", key);
    skip(); // Skip the value so we can look at the next key

    SIMDJSON_TRY( has_next_field().get(has_next) );
  } while (has_next);
  logger::log_event(*this, "no matches", key);
  return false;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<raw_json_string> json_iterator::field_key() noexcept {
  const uint8_t *key = advance();
  if (*(key++) != '"') { logger::log_error(*this, "Object key is not a string"); return TAPE_ERROR; }
  return raw_json_string(key);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::field_value() noexcept {
  if (*advance() != ':') { logger::log_error(*this, "Missing colon in object field"); return TAPE_ERROR; }
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::start_array() noexcept {
  if (*advance() != '[') { logger::log_error(*this, "Not an array"); return INCORRECT_TYPE; }
  return started_array();
}

SIMDJSON_WARN_UNUSED simdjson_really_inline bool json_iterator::started_array() noexcept {
  if (*peek() == ']') {
    logger::log_value(*this, "empty array");
    advance();
    return false;
  }
  logger::log_start_value(*this, "array"); 
  return true;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::has_next_element() noexcept {
  switch (*advance()) {
    case ']':
      logger::log_end_value(*this, "array");
      return false;
    case ',':
      return true;
    default:
      logger::log_error(*this, "Missing comma between array elements");
      return TAPE_ERROR;
  }
}

SIMDJSON_WARN_UNUSED simdjson_result<raw_json_string> json_iterator::get_raw_json_string() noexcept {
  logger::log_value(*this, "string", "", 0);
  return raw_json_string(advance()+1);
}
SIMDJSON_WARN_UNUSED simdjson_result<uint64_t> json_iterator::get_uint64() noexcept {
  logger::log_value(*this, "uint64", "", 0);
  return stage2::numberparsing::parse_unsigned(advance());
}
SIMDJSON_WARN_UNUSED simdjson_result<int64_t> json_iterator::get_int64() noexcept {
  logger::log_value(*this, "int64", "", 0);
  return stage2::numberparsing::parse_integer(advance());
}
SIMDJSON_WARN_UNUSED simdjson_result<double> json_iterator::get_double() noexcept {
  logger::log_value(*this, "double", "", 0);
  return stage2::numberparsing::parse_double(advance());
}
SIMDJSON_WARN_UNUSED simdjson_result<bool> json_iterator::get_bool() noexcept {
  logger::log_value(*this, "bool", "", 0);
  auto json = advance();
  auto not_true = stage2::atomparsing::str4ncmp(json, "true");
  auto not_false = stage2::atomparsing::str4ncmp(json, "fals") | (json[4] ^ 'e');
  bool error = (not_true && not_false) || stage2::is_not_structural_or_whitespace(json[not_true ? 5 : 4]);
  if (error) { logger::log_error(*this, "not a boolean"); return INCORRECT_TYPE; }
  return simdjson_result<bool>(!not_true);
}
simdjson_really_inline bool json_iterator::is_null() noexcept {
  auto json = peek();
  if (!stage2::atomparsing::str4ncmp(json, "null")) {
    logger::log_value(*this, "null", "", 0);
    advance();
    return true;
  }
  return false;
}

template<int N>
SIMDJSON_WARN_UNUSED simdjson_really_inline bool json_iterator::advance_to_buffer(uint8_t (&tmpbuf)[N]) noexcept {
  // Truncate whitespace to fit the buffer.
  auto len = peek_length();
  auto json = advance();
  if (len > N-1) {
    if (stage2::is_not_structural_or_whitespace(json[N])) { return false; }
    len = N-1;
  }

  // Copy to the buffer.
  memcpy(tmpbuf, json, len);
  tmpbuf[len] = ' ';
  return true;
}

constexpr const uint32_t MAX_INT_LENGTH = 1024;

SIMDJSON_WARN_UNUSED simdjson_result<uint64_t> json_iterator::get_root_uint64() noexcept {
  uint8_t tmpbuf[20+1]; // <20 digits> is the longest possible unsigned integer
  if (!advance_to_buffer(tmpbuf)) { return NUMBER_ERROR; }
  logger::log_value(*this, "uint64", "", 0);
  return stage2::numberparsing::parse_unsigned(buf);
}
SIMDJSON_WARN_UNUSED simdjson_result<int64_t> json_iterator::get_root_int64() noexcept {
  uint8_t tmpbuf[20+1]; // -<19 digits> is the longest possible integer 
  if (!advance_to_buffer(tmpbuf)) { return NUMBER_ERROR; }
  logger::log_value(*this, "int64", "", 0);
  return stage2::numberparsing::parse_integer(buf);
}
SIMDJSON_WARN_UNUSED simdjson_result<double> json_iterator::get_root_double() noexcept {
  // Per https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/, 1074 is the maximum number of significant fractional digits. Add 8 more digits for the biggest number: -0.<fraction>e-308.
  uint8_t tmpbuf[1074+8+1];
  if (!advance_to_buffer(tmpbuf)) { return NUMBER_ERROR; }
  logger::log_value(*this, "double", "", 0);
  return stage2::numberparsing::parse_double(buf);
}
SIMDJSON_WARN_UNUSED simdjson_result<bool> json_iterator::get_root_bool() noexcept {
  uint8_t tmpbuf[5+1];
  if (!advance_to_buffer(tmpbuf)) { return INCORRECT_TYPE; } // Too big! Can't be true or false
  return get_bool();
}
simdjson_really_inline bool json_iterator::root_is_null() noexcept {
  uint8_t tmpbuf[4+1];
  if (!advance_to_buffer(tmpbuf)) { return false; } // Too big! Can't be null
  return is_null();
}


simdjson_really_inline void json_iterator::skip() noexcept {
  uint32_t depth = 0;
  do {
    switch (*advance()) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}':
        logger::log_end_value(*this, "skip");
        depth--;
        break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{':
        logger::log_start_value(*this, "skip");
        depth++;
        break;
      default:
        logger::log_value(*this, "skip", "");
        break;
    }
  } while (depth > 0);
}

simdjson_really_inline bool json_iterator::skip_container() noexcept {
  uint32_t depth = 1;
  // The loop breaks only when depth-- happens.
  while (true) {
    uint8_t ch = *advance();
    switch (ch) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}':
        logger::log_end_value(*this, "skip");
        depth--;
        if (depth == 0) { logger::log_event(*this, "end skip", ""); return ch == ']'; }
        break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{':
        logger::log_start_value(*this, "skip");
        depth++;
        break;
      default:
        logger::log_value(*this, "skip", "");
        break;
    }
  };
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
