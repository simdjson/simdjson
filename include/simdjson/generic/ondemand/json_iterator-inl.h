namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline json_iterator::json_iterator(json_iterator &&other) noexcept
  : token_iterator(std::forward<token_iterator>(other)),
    parser{other.parser},
    current_string_buf_loc{other.current_string_buf_loc}
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
    , active_lease_depth{other.active_lease_depth}
#endif
{
  other.parser = nullptr;
}
simdjson_really_inline json_iterator &json_iterator::operator=(json_iterator &&other) noexcept {
  buf = other.buf;
  index = other.index;
  parser = other.parser;
  current_string_buf_loc = other.current_string_buf_loc;
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  active_lease_depth = other.active_lease_depth;
#endif
  other.parser = nullptr;
  return *this;
}

simdjson_really_inline json_iterator::json_iterator(ondemand::parser *_parser) noexcept
  : token_iterator(_parser->dom_parser.buf, _parser->dom_parser.structural_indexes.get()),
    parser{_parser},
    current_string_buf_loc{parser->string_buf.get()}
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
    , active_lease_depth{0}
#endif
{
  // Release the string buf so it can be reused by the next document
  logger::log_headers();
}
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
simdjson_really_inline json_iterator::~json_iterator() noexcept {
  // If we have any leases out when we die, it's an error
  SIMDJSON_ASSUME(active_lease_depth == 0);
}
#endif

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> json_iterator::start_object(const uint8_t *json) noexcept {
  if (*json != '{') { logger::log_error(*this, "Not an object"); return INCORRECT_TYPE; }
  return started_object();
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> json_iterator::start_object() noexcept {
  return start_object(advance());
}

simdjson_warn_unused simdjson_really_inline bool json_iterator::started_object() noexcept {
  if (*peek() == '}') {
    logger::log_value(*this, "empty object");
    advance();
    return false;
  }
  logger::log_start_value(*this, "object");
  return true;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> json_iterator::has_next_field() noexcept {
  switch (*advance()) {
    case '}':
      logger::log_end_value(*this, "object");
      return false;
    case ',':
      return true;
    default:
      return report_error(TAPE_ERROR, "Missing comma between object fields");
  }
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> json_iterator::find_field_raw(const char *key) noexcept {
  bool has_next;
  do {
    raw_json_string actual_key;
    SIMDJSON_TRY( consume_raw_json_string().get(actual_key) );
    if (*advance() != ':') { return report_error(TAPE_ERROR, "Missing colon in object field"); }
    if (actual_key == key) {
      logger::log_event(*this, "match", key);
      return true;
    }
    logger::log_event(*this, "non-match", key);
    SIMDJSON_TRY( skip() ); // Skip the value so we can look at the next key

    SIMDJSON_TRY( has_next_field().get(has_next) );
  } while (has_next);
  logger::log_event(*this, "no matches", key);
  return false;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> json_iterator::field_key() noexcept {
  const uint8_t *key = advance();
  if (*(key++) != '"') { return report_error(TAPE_ERROR, "Object key is not a string"); }
  return raw_json_string(key);
}

simdjson_warn_unused simdjson_really_inline error_code json_iterator::field_value() noexcept {
  if (*advance() != ':') { return report_error(TAPE_ERROR, "Missing colon in object field"); }
  return SUCCESS;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> json_iterator::start_array(const uint8_t *json) noexcept {
  if (*json != '[') { logger::log_error(*this, "Not an array"); return INCORRECT_TYPE; }
  return started_array();
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> json_iterator::start_array() noexcept {
  return start_array(advance());
}

simdjson_warn_unused simdjson_really_inline bool json_iterator::started_array() noexcept {
  if (*peek() == ']') {
    logger::log_value(*this, "empty array");
    advance();
    return false;
  }
  logger::log_start_value(*this, "array"); 
  return true;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> json_iterator::has_next_element() noexcept {
  switch (*advance()) {
    case ']':
      logger::log_end_value(*this, "array");
      return false;
    case ',':
      return true;
    default:
      return report_error(TAPE_ERROR, "Missing comma between array elements");
  }
}

simdjson_warn_unused simdjson_result<std::string_view> json_iterator::parse_string(const uint8_t *json) noexcept {
  return parse_raw_json_string(json).unescape(current_string_buf_loc);
}
simdjson_warn_unused simdjson_result<std::string_view> json_iterator::consume_string() noexcept {
  return parse_string(advance());
}
simdjson_warn_unused simdjson_result<raw_json_string> json_iterator::parse_raw_json_string(const uint8_t *json) noexcept {
  logger::log_value(*this, "string", "");
  if (*json != '"') { logger::log_error(*this, "Not a string"); return INCORRECT_TYPE; }
  return raw_json_string(json+1);
}
simdjson_warn_unused simdjson_result<raw_json_string> json_iterator::consume_raw_json_string() noexcept {
  return parse_raw_json_string(advance());
}
simdjson_warn_unused simdjson_result<uint64_t> json_iterator::parse_uint64(const uint8_t *json) noexcept {
  logger::log_value(*this, "uint64", "");
  return numberparsing::parse_unsigned(json);
}
simdjson_warn_unused simdjson_result<uint64_t> json_iterator::consume_uint64() noexcept {
  return parse_uint64(advance());
}
simdjson_warn_unused simdjson_result<int64_t> json_iterator::parse_int64(const uint8_t *json) noexcept {
  logger::log_value(*this, "int64", "");
  return numberparsing::parse_integer(json);
}
simdjson_warn_unused simdjson_result<int64_t> json_iterator::consume_int64() noexcept {
  return parse_int64(advance());
}
simdjson_warn_unused simdjson_result<double> json_iterator::parse_double(const uint8_t *json) noexcept {
  logger::log_value(*this, "double", "");
  return numberparsing::parse_double(json);
}
simdjson_warn_unused simdjson_result<double> json_iterator::consume_double() noexcept {
  return parse_double(advance());
}
simdjson_warn_unused simdjson_result<bool> json_iterator::parse_bool(const uint8_t *json) noexcept {
  logger::log_value(*this, "bool", "");
  auto not_true = atomparsing::str4ncmp(json, "true");
  auto not_false = atomparsing::str4ncmp(json, "fals") | (json[4] ^ 'e');
  bool error = (not_true && not_false) || jsoncharutils::is_not_structural_or_whitespace(json[not_true ? 5 : 4]);
  if (error) { logger::log_error(*this, "Not a boolean"); return INCORRECT_TYPE; }
  return simdjson_result<bool>(!not_true);
}
simdjson_warn_unused simdjson_result<bool> json_iterator::consume_bool() noexcept {
  return parse_bool(advance());
}
simdjson_really_inline bool json_iterator::is_null(const uint8_t *json) noexcept {
  if (!atomparsing::str4ncmp(json, "null")) {
    logger::log_value(*this, "null", "");
    return true;
  }
  return false;
}
simdjson_really_inline bool json_iterator::is_null() noexcept {
  if (is_null(peek())) {
    advance();
    return true;
  }
  return false;
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::copy_to_buffer(const uint8_t *json, uint8_t (&tmpbuf)[N]) noexcept {
  // Truncate whitespace to fit the buffer.
  auto len = peek_length(-1);
  if (len > N-1) {
    if (jsoncharutils::is_not_structural_or_whitespace(json[N])) { return false; }
    len = N-1;
  }

  // Copy to the buffer.
  memcpy(tmpbuf, json, len);
  tmpbuf[len] = ' ';
  return true;
}

constexpr const uint32_t MAX_INT_LENGTH = 1024;

simdjson_warn_unused simdjson_result<uint64_t> json_iterator::parse_root_uint64(const uint8_t *json) noexcept {
  uint8_t tmpbuf[20+1]; // <20 digits> is the longest possible unsigned integer
  if (!copy_to_buffer(json, tmpbuf)) { logger::log_error(*this, "Root number more than 20 characters"); return NUMBER_ERROR; }
  logger::log_value(*this, "uint64", "");
  auto result = numberparsing::parse_unsigned(tmpbuf);
  if (result.error()) { logger::log_error(*this, "Error parsing unsigned integer"); return result.error(); }
  return result;
}
simdjson_warn_unused simdjson_result<uint64_t> json_iterator::consume_root_uint64() noexcept {
  return parse_root_uint64(advance());
}
simdjson_warn_unused simdjson_result<int64_t> json_iterator::parse_root_int64(const uint8_t *json) noexcept {
  uint8_t tmpbuf[20+1]; // -<19 digits> is the longest possible integer 
  if (!copy_to_buffer(json, tmpbuf)) { logger::log_error(*this, "Root number more than 20 characters"); return NUMBER_ERROR; }
  logger::log_value(*this, "int64", "");
  auto result = numberparsing::parse_integer(tmpbuf);
  if (result.error()) { report_error(result.error(), "Error parsing integer"); }
  return result;
}
simdjson_warn_unused simdjson_result<int64_t> json_iterator::consume_root_int64() noexcept {
  return parse_root_int64(advance());
}
simdjson_warn_unused simdjson_result<double> json_iterator::parse_root_double(const uint8_t *json) noexcept {
  // Per https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/, 1074 is the maximum number of significant fractional digits. Add 8 more digits for the biggest number: -0.<fraction>e-308.
  uint8_t tmpbuf[1074+8+1];
  if (!copy_to_buffer(json, tmpbuf)) { logger::log_error(*this, "Root number more than 1082 characters"); return NUMBER_ERROR; }
  logger::log_value(*this, "double", "");
  auto result = numberparsing::parse_double(tmpbuf);
  if (result.error()) { report_error(result.error(), "Error parsing double"); }
  return result;
}
simdjson_warn_unused simdjson_result<double> json_iterator::consume_root_double() noexcept {
  return parse_root_double(advance());
}
simdjson_warn_unused simdjson_result<bool> json_iterator::parse_root_bool(const uint8_t *json) noexcept {
  uint8_t tmpbuf[5+1];
  if (!copy_to_buffer(json, tmpbuf)) { logger::log_error(*this, "Not a boolean"); return INCORRECT_TYPE; }
  return parse_bool(tmpbuf);
}
simdjson_warn_unused simdjson_result<bool> json_iterator::consume_root_bool() noexcept {
  return parse_root_bool(advance());
}
simdjson_really_inline bool json_iterator::root_is_null(const uint8_t *json) noexcept {
  uint8_t tmpbuf[4+1];
  if (!copy_to_buffer(json, tmpbuf)) { return false; }
  return is_null(tmpbuf);
}

simdjson_warn_unused simdjson_really_inline error_code json_iterator::skip() noexcept {
  switch (*advance()) {
    // PERF TODO does it skip the depth check when we don't decrement depth?
    case '[': case '{':
      logger::log_start_value(*this, "skip");
      return skip_container();
    default:
      logger::log_value(*this, "skip", "");
      return SUCCESS;
  }
}

simdjson_warn_unused simdjson_really_inline error_code json_iterator::skip_container() noexcept {
  uint32_t depth = 1;
  // The loop breaks only when depth-- happens.
  auto end = &parser->dom_parser.structural_indexes[parser->dom_parser.n_structural_indexes];
  while (index <= end) {
    uint8_t ch = *advance();
    switch (ch) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}':
        logger::log_end_value(*this, "skip");
        depth--;
        if (depth == 0) { logger::log_event(*this, "end skip", ""); return SUCCESS; }
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
  }

  return report_error(TAPE_ERROR, "not enough close braces");
}

simdjson_really_inline bool json_iterator::at_start() const noexcept {
  return index == parser->dom_parser.structural_indexes.get();
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return index == &parser->dom_parser.structural_indexes[parser->dom_parser.n_structural_indexes];
}

simdjson_really_inline bool json_iterator::is_alive() const noexcept {
  return parser;
}


simdjson_really_inline json_iterator_ref json_iterator::borrow() noexcept {
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  SIMDJSON_ASSUME(active_lease_depth == 0);
  const uint32_t child_depth = 1;
  active_lease_depth = child_depth;
  return json_iterator_ref(this, child_depth);
#else
  return json_iterator_ref(this);
#endif
}

simdjson_really_inline error_code json_iterator::report_error(error_code error, const char *message) noexcept {
  SIMDJSON_ASSUME(error != SUCCESS && error != UNINITIALIZED && error != INCORRECT_TYPE && error != NO_SUCH_FIELD);
  logger::log_error(*this, message);
  _error = error;
  return error;
}
simdjson_really_inline error_code json_iterator::error() const noexcept {
  return _error;
}

//
// json_iterator_ref
//
simdjson_really_inline json_iterator_ref::json_iterator_ref(json_iterator_ref &&other) noexcept
  : iter{other.iter}
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
    , lease_depth{other.lease_depth}
#endif // SIMDJSON_ONDEMAND_SAFETY_RAILS
{
  other.iter = nullptr;
}
simdjson_really_inline json_iterator_ref &json_iterator_ref::operator=(json_iterator_ref &&other) noexcept {
  assert_is_not_active();
  iter = other.iter;
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  lease_depth = other.lease_depth;
#endif // SIMDJSON_ONDEMAND_SAFETY_RAILS
  other.iter = nullptr;
  return *this;
}

#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
simdjson_really_inline json_iterator_ref::~json_iterator_ref() noexcept {
  // The caller MUST consume their value and release the iterator before they die
  assert_is_not_active();
}
simdjson_really_inline json_iterator_ref::json_iterator_ref(
  json_iterator *_iter,
  uint32_t _lease_depth
) noexcept : iter{_iter}, lease_depth{_lease_depth}
{
  assert_is_active();
}
#else
simdjson_really_inline json_iterator_ref::json_iterator_ref(
  json_iterator *_iter
) noexcept : iter{_iter}
{
  assert_is_active();
}
#endif // SIMDJSON_ONDEMAND_SAFETY_RAILS

simdjson_really_inline json_iterator_ref json_iterator_ref::borrow() noexcept {
  assert_is_active();
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  const uint32_t child_depth = lease_depth + 1;
  iter->active_lease_depth = child_depth;
  return json_iterator_ref(iter, child_depth);
#else
  return json_iterator_ref(iter);
#endif
}
simdjson_really_inline void json_iterator_ref::release() noexcept {
  assert_is_active();
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  iter->active_lease_depth = lease_depth - 1;
#endif
  iter = nullptr;
}

simdjson_really_inline json_iterator *json_iterator_ref::operator->() noexcept {
  assert_is_active();
  return iter;
}
simdjson_really_inline json_iterator &json_iterator_ref::operator*() noexcept {
  assert_is_active();
  return *iter;
}
simdjson_really_inline const json_iterator &json_iterator_ref::operator*() const noexcept {
  assert_is_active();
  return *iter;
}

simdjson_really_inline bool json_iterator_ref::is_alive() const noexcept {
  return iter != nullptr;
}
simdjson_really_inline bool json_iterator_ref::is_active() const noexcept {
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  return is_alive() && lease_depth == iter->active_lease_depth;
#else
  return is_alive();
#endif
}
simdjson_really_inline void json_iterator_ref::assert_is_active() const noexcept {
// We don't call const functions because VC++ is worried they might have side effects in __assume
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  SIMDJSON_ASSUME(iter != nullptr && lease_depth == iter->active_lease_depth);
#else
  SIMDJSON_ASSUME(iter != nullptr);
#endif
}
simdjson_really_inline void json_iterator_ref::assert_is_not_active() const noexcept {
// We don't call const functions because VC++ is worried they might have side effects in __assume
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  SIMDJSON_ASSUME(!(iter != nullptr && lease_depth == iter->active_lease_depth));
#else
  SIMDJSON_ASSUME(!(iter != nullptr));
#endif
}



} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(error) {}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator_ref>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator_ref &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator_ref>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator_ref>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator_ref>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator_ref>(error) {}

} // namespace simdjson