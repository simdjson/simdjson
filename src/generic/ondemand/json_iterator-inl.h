namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline json_iterator::json_iterator() noexcept = default;
simdjson_really_inline json_iterator::json_iterator(json_iterator &&other) noexcept = default;
simdjson_really_inline json_iterator &json_iterator::operator=(json_iterator &&other) noexcept = default;
simdjson_really_inline json_iterator::json_iterator(const uint8_t *_buf, uint32_t *_index, uint32_t _depth) noexcept
  : token_iterator(_buf, _index), depth{_depth}
{
}


SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<json_iterator::container> json_iterator::start_object() noexcept {
  if (*advance() != '{') { logger::log_error(*this, "Not an object"); return INCORRECT_TYPE; }
  return started_object();
}

SIMDJSON_WARN_UNUSED simdjson_really_inline json_iterator::container json_iterator::started_object() noexcept {
  depth++;
  return depth;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline bool json_iterator::is_empty_object() noexcept {
  if (*peek() == '}') {
    advance();
    depth--;
    logger::log_value(*this, "empty object", "", -2);
    return true;
  }
  logger::log_start_value(*this, "object", -1, -1); 
  return false;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::has_next_field() noexcept {
  switch (*advance()) {
    case '}':
      depth--;
      logger::log_end_value(*this, "object");
      return false;
    case ',':
      return true;
    default:
      logger::log_error(*this, "Missing comma between object fields");
      return TAPE_ERROR;
  }
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::next_field(container c) noexcept {
  skip_unfinished_children(c);
  return has_next_field();
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

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::find_single_field_raw(const char *key) noexcept {
  auto error = start_object().error();
  if (error) { return error; }
  return find_first_field_raw(key);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::find_first_field_raw(const char *key) noexcept {
  if (is_empty_object()) { return false; }
  return find_field_raw(key);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::find_next_field_raw(const char *key, container c) noexcept {
  bool has_next;
  SIMDJSON_TRY( next_field(c).get(has_next) );
  if (!has_next) { return false; }

  return find_field_raw(key);
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

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<json_iterator::container> json_iterator::start_array() noexcept {
  if (*advance() != '[') { logger::log_error(*this, "Not an array"); return INCORRECT_TYPE; }
  return started_array();
}

SIMDJSON_WARN_UNUSED simdjson_really_inline json_iterator::container json_iterator::started_array() noexcept {
  depth++;
  return depth;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline bool json_iterator::is_empty_array() noexcept {
  if (*peek() == ']') {
    advance();
    depth--;
    logger::log_value(*this, "empty array", "", -2);
    return true;
  }
  logger::log_start_value(*this, "array", -1, -1); 
  return false;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::has_next_element() noexcept {
  switch (*advance()) {
    case ']':
      depth--;
      logger::log_end_value(*this, "array");
      return false;
    case ',':
      return true;
    default:
      logger::log_error(*this, "Missing comma between array elements");
      return TAPE_ERROR;
  }
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::next_element(container c) noexcept {
  skip_unfinished_children(c);
  return has_next_element();
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

simdjson_really_inline void json_iterator::skip_unfinished_children(container c) noexcept {
  SIMDJSON_ASSUME(depth >= c.depth);
  while (depth > c.depth) {
    switch (*advance()) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}': depth--; logger::log_end_value(*this, "skip"); break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{': logger::log_start_value(*this, "skip"); depth++; break;
      default: logger::log_value(*this, "skip", ""); break;
    }
  }
}

simdjson_really_inline void json_iterator::finish(container c) noexcept {
  while (depth >= c.depth) {
    switch (*advance()) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}': depth--; logger::log_end_value(*this, "skip"); break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{': logger::log_start_value(*this, "skip"); depth++; break;
      default: logger::log_value(*this, "skip", ""); break;
    }
  }
  SIMDJSON_ASSUME(depth == c.depth-1);
}


simdjson_really_inline void json_iterator::skip() noexcept {
  uint32_t child_depth = 0;
  do {
    switch (*advance()) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}': child_depth--; logger::log_end_value(*this, "skip", -1, child_depth); break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{': logger::log_start_value(*this, "skip", -1, child_depth); child_depth++; break;
      default: logger::log_value(*this, "skip", "", -1, child_depth); break;
    }
  } while (child_depth > 0);
}

simdjson_really_inline bool json_iterator::in_container(container c) const noexcept {
  return depth >= c.depth;
}

simdjson_really_inline json_iterator::container json_iterator::current_container() const noexcept {
  return depth;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
