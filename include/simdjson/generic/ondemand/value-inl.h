namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline value::value(json_iterator_ref && _iter, const uint8_t *_json) noexcept
  : iter{std::forward<json_iterator_ref>(_iter)},
    json{_json}
{
  iter.assert_is_active();
  SIMDJSON_ASSUME(json != nullptr);
}

simdjson_really_inline value::~value() noexcept {
  // If the user didn't actually use the value, we need to check if it's an array/object and bump
  // depth so that the array/object iteration routines will work correctly.
  // PERF TODO this better be elided entirely when people actually use the value. Don't care if it
  // gets bumped on the error path unless that's costing us something important.
  if (iter.is_alive()) {
    if (*json == '[' || *json == '{') {
      logger::log_start_value(*iter, "unused");
      SIMDJSON_UNUSED auto _err = iter->skip_container();
    } else {
      logger::log_value(*iter, "unused");
    }
    iter.release();
  }
}

simdjson_really_inline value value::start(json_iterator_ref &&iter) noexcept {
  return { std::forward<json_iterator_ref>(iter), iter->advance() };
}

simdjson_really_inline simdjson_result<array> value::get_array() && noexcept {
  if (*json != '[') {
    log_error("not an array");
    iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
    return INCORRECT_TYPE;
  }
  return array::started(std::forward<json_iterator_ref>(iter));
}
simdjson_really_inline simdjson_result<object> value::get_object() && noexcept {
  if (*json != '{') {
    log_error("not an object");
    iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
    return INCORRECT_TYPE;
  }
  return object::started(std::forward<json_iterator_ref>(iter));
}
simdjson_really_inline simdjson_result<raw_json_string> value::get_raw_json_string() && noexcept {
  log_value("string");
  iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
  if (*json != '"') { log_error("not a string"); return INCORRECT_TYPE; }
  return raw_json_string{&json[1]};
}
simdjson_really_inline simdjson_result<std::string_view> value::get_string() && noexcept {
  log_value("string");
  if (*json != '"') {
    log_error("not a string");
    iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
    return INCORRECT_TYPE;
  }
  auto result = raw_json_string{&json[1]}.unescape(iter->current_string_buf_loc);
  iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
  return result;
}
simdjson_really_inline simdjson_result<double> value::get_double() && noexcept {
  log_value("double");
  iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
  double result;
  error_code error;
  if ((error = numberparsing::parse_double(json).get(result))) { log_error("not a double"); return error; }
  return result;
}
simdjson_really_inline simdjson_result<uint64_t> value::get_uint64() && noexcept {
  log_value("unsigned");
  iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
  uint64_t result;
  error_code error;
  if ((error = numberparsing::parse_unsigned(json).get(result))) { log_error("not a unsigned integer"); return error; }
  return result;
}
simdjson_really_inline simdjson_result<int64_t> value::get_int64() && noexcept {
  log_value("integer");
  iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
  int64_t result;
  error_code error;
  if ((error = numberparsing::parse_integer(json).get(result))) { log_error("not an integer"); return error; }
  return result;
}
simdjson_really_inline simdjson_result<bool> value::get_bool() && noexcept {
  log_value("bool");
  iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
  auto not_true = atomparsing::str4ncmp(json, "true");
  auto not_false = atomparsing::str4ncmp(json, "fals") | (json[4] ^ 'e');
  bool error = (not_true && not_false) || jsoncharutils::is_not_structural_or_whitespace(json[not_true ? 5 : 4]);
  if (error) { log_error("not a boolean"); return INCORRECT_TYPE; }
  return simdjson_result<bool>(!not_true, error ? INCORRECT_TYPE : SUCCESS);
}
simdjson_really_inline bool value::is_null() & noexcept {
  log_value("null");
  // Since it's a *reference*, we may want to check something other than is_null() if it isn't null,
  // so we don't release the iterator unless it is actually null
  if (atomparsing::str4ncmp(json, "null")) { return false; }
  iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
  return true;
}
simdjson_really_inline bool value::is_null() && noexcept {
  log_value("null");
  iter.release(); // Communicate that we have handled the value PERF TODO elided, right?
  if (atomparsing::str4ncmp(json, "null")) { return false; }
  return true;
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline value::operator array() && noexcept(false) {
  return std::forward<value>(*this).get_array();
}
simdjson_really_inline value::operator object() && noexcept(false) {
  return std::forward<value>(*this).get_object();
}
simdjson_really_inline value::operator uint64_t() && noexcept(false) {
  return std::forward<value>(*this).get_uint64();
}
simdjson_really_inline value::operator int64_t() && noexcept(false) {
  return std::forward<value>(*this).get_int64();
}
simdjson_really_inline value::operator double() && noexcept(false) {
  return std::forward<value>(*this).get_double();
}
simdjson_really_inline value::operator std::string_view() && noexcept(false) {
  return std::forward<value>(*this).get_string();
}
simdjson_really_inline value::operator raw_json_string() && noexcept(false) {
  return std::forward<value>(*this).get_raw_json_string();
}
simdjson_really_inline value::operator bool() && noexcept(false) {
  return std::forward<value>(*this).get_bool();
}
#endif

simdjson_really_inline simdjson_result<array_iterator> value::begin() & noexcept {
  if (*json != '[') {
    log_error("not an array");
    return INCORRECT_TYPE;
  }
  if (!iter->started_array()) { iter.release(); }
  return array_iterator(iter);
}
simdjson_really_inline simdjson_result<array_iterator> value::end() & noexcept {
  return {};
}
simdjson_really_inline simdjson_result<value> value::operator[](std::string_view key) && noexcept {
  return std::forward<value>(*this).get_object()[key];
}
simdjson_really_inline simdjson_result<value> value::operator[](const char *key) && noexcept {
  return std::forward<value>(*this).get_object()[key];
}

simdjson_really_inline void value::log_value(const char *type) const noexcept {
  char json_char[]{char(json[0]), '\0'};
  logger::log_value(*iter, type, json_char);
}
simdjson_really_inline void value::log_error(const char *message) const noexcept {
  char json_char[]{char(json[0]), '\0'};
  logger::log_error(*iter, message, json_char);
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::value &&value
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::simdjson_result(
  error_code error
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value>(error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::begin() & noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::end() & noexcept {
  if (error()) { return error(); }
  return {};
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator[](std::string_view key) && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first)[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator[](const char *key) && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first)[key];
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_array() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_array();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_object() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_object();
}
simdjson_really_inline simdjson_result<uint64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_uint64() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_uint64();
}
simdjson_really_inline simdjson_result<int64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_int64() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_int64();
}
simdjson_really_inline simdjson_result<double> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_double() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_double();
}
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_string() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_string();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_raw_json_string() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_bool() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_bool();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::is_null() & noexcept {
  if (error()) { return false; }
  return first.is_null();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::is_null() && noexcept {
  if (error()) { return false; }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).is_null();
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_IMPLEMENTATION::ondemand::array() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_IMPLEMENTATION::ondemand::object() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator uint64_t() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator int64_t() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator double() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator std::string_view() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator bool() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
#endif

} // namespace simdjson
