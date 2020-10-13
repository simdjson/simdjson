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
      simdjson_unused auto _err = iter->skip_container();
    } else {
      logger::log_value(*iter, "unused");
    }
    iter.release();
  }
}

simdjson_really_inline value value::start(json_iterator_ref &&iter) noexcept {
  return { std::forward<json_iterator_ref>(iter), iter->advance() };
}

simdjson_really_inline const uint8_t *value::consume() noexcept {
  iter.release();
  return json;
}
template<typename T>
simdjson_really_inline simdjson_result<T> value::consume_if_success(simdjson_result<T> &&result) noexcept {
  if (!result.error()) { consume(); }
  return std::forward<simdjson_result<T>>(result);
}

simdjson_really_inline simdjson_result<array> value::get_array() noexcept {
  bool has_value;
  SIMDJSON_TRY( iter->start_array(json).get(has_value) );
  if (!has_value) { iter.release(); }
  return array(std::move(iter));
}
simdjson_really_inline simdjson_result<object> value::get_object() noexcept {
  bool has_value;
  SIMDJSON_TRY( iter->start_object(json).get(has_value) );
  if (!has_value) { iter.release(); }
  return object(std::move(iter));
}
simdjson_really_inline simdjson_result<raw_json_string> value::get_raw_json_string() && noexcept {
  return iter->consume_raw_json_string();
}
simdjson_really_inline simdjson_result<raw_json_string> value::get_raw_json_string() & noexcept {
  return consume_if_success( iter->parse_raw_json_string(json) );
}
simdjson_really_inline simdjson_result<std::string_view> value::get_string() && noexcept {
  auto result = iter->parse_string(json);
  consume();
  return result;
}
simdjson_really_inline simdjson_result<std::string_view> value::get_string() & noexcept {
  return consume_if_success( iter->parse_string(json) );
}
simdjson_really_inline simdjson_result<double> value::get_double() && noexcept {
  return iter->parse_double(consume());
}
simdjson_really_inline simdjson_result<double> value::get_double() & noexcept {
  return consume_if_success( iter->parse_double(json) );
}
simdjson_really_inline simdjson_result<uint64_t> value::get_uint64() && noexcept {
  return iter->parse_uint64(consume());
}
simdjson_really_inline simdjson_result<uint64_t> value::get_uint64() & noexcept {
  return consume_if_success( iter->parse_uint64(json) );
}
simdjson_really_inline simdjson_result<int64_t> value::get_int64() && noexcept {
  return iter->parse_int64(consume());
}
simdjson_really_inline simdjson_result<int64_t> value::get_int64() & noexcept {
  return consume_if_success( iter->parse_int64(json) );
}
simdjson_really_inline simdjson_result<bool> value::get_bool() && noexcept {
  return iter->parse_bool(consume());
}
simdjson_really_inline simdjson_result<bool> value::get_bool() & noexcept {
  return consume_if_success( iter->parse_bool(json) );
}
simdjson_really_inline bool value::is_null() && noexcept {
  return iter->is_null(consume());
}
simdjson_really_inline bool value::is_null() & noexcept {
  if (!iter->is_null(json)) { return false; }
  consume();
  return true;
}

template<> simdjson_really_inline simdjson_result<array> value::get() & noexcept { return get_array(); }
template<> simdjson_really_inline simdjson_result<object> value::get() & noexcept { return get_object(); }
template<> simdjson_really_inline simdjson_result<raw_json_string> value::get() & noexcept { return get_raw_json_string(); }
template<> simdjson_really_inline simdjson_result<std::string_view> value::get() & noexcept { return get_string(); }
template<> simdjson_really_inline simdjson_result<double> value::get() & noexcept { return get_double(); }
template<> simdjson_really_inline simdjson_result<uint64_t> value::get() & noexcept { return get_uint64(); }
template<> simdjson_really_inline simdjson_result<int64_t> value::get() & noexcept { return get_int64(); }
template<> simdjson_really_inline simdjson_result<bool> value::get() & noexcept { return get_bool(); }

template<> simdjson_really_inline simdjson_result<value> value::get() && noexcept { return std::forward<value>(*this); }
template<> simdjson_really_inline simdjson_result<array> value::get() && noexcept { return std::forward<value>(*this).get_array(); }
template<> simdjson_really_inline simdjson_result<object> value::get() && noexcept { return std::forward<value>(*this).get_object(); }
template<> simdjson_really_inline simdjson_result<raw_json_string> value::get() && noexcept { return std::forward<value>(*this).get_raw_json_string(); }
template<> simdjson_really_inline simdjson_result<std::string_view> value::get() && noexcept { return std::forward<value>(*this).get_string(); }
template<> simdjson_really_inline simdjson_result<double> value::get() && noexcept { return std::forward<value>(*this).get_double(); }
template<> simdjson_really_inline simdjson_result<uint64_t> value::get() && noexcept { return std::forward<value>(*this).get_uint64(); }
template<> simdjson_really_inline simdjson_result<int64_t> value::get() && noexcept { return std::forward<value>(*this).get_int64(); }
template<> simdjson_really_inline simdjson_result<bool> value::get() && noexcept { return std::forward<value>(*this).get_bool(); }

template<typename T> simdjson_really_inline error_code value::get(T &out) & noexcept {
  return get<T>().get(out);
}
template<typename T> simdjson_really_inline error_code value::get(T &out) && noexcept {
  return std::forward<value>(*this).get<T>().get(out);
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline value::operator array() noexcept(false) {
  return std::forward<value>(*this).get_array();
}
simdjson_really_inline value::operator object() noexcept(false) {
  return std::forward<value>(*this).get_object();
}
simdjson_really_inline value::operator uint64_t() && noexcept(false) {
  return std::forward<value>(*this).get_uint64();
}
simdjson_really_inline value::operator uint64_t() & noexcept(false) {
  return std::forward<value>(*this).get_uint64();
}
simdjson_really_inline value::operator int64_t() && noexcept(false) {
  return std::forward<value>(*this).get_int64();
}
simdjson_really_inline value::operator int64_t() & noexcept(false) {
  return std::forward<value>(*this).get_int64();
}
simdjson_really_inline value::operator double() && noexcept(false) {
  return std::forward<value>(*this).get_double();
}
simdjson_really_inline value::operator double() & noexcept(false) {
  return std::forward<value>(*this).get_double();
}
simdjson_really_inline value::operator std::string_view() && noexcept(false) {
  return std::forward<value>(*this).get_string();
}
simdjson_really_inline value::operator std::string_view() & noexcept(false) {
  return std::forward<value>(*this).get_string();
}
simdjson_really_inline value::operator raw_json_string() && noexcept(false) {
  return std::forward<value>(*this).get_raw_json_string();
}
simdjson_really_inline value::operator raw_json_string() & noexcept(false) {
  return std::forward<value>(*this).get_raw_json_string();
}
simdjson_really_inline value::operator bool() && noexcept(false) {
  return std::forward<value>(*this).get_bool();
}
simdjson_really_inline value::operator bool() & noexcept(false) {
  return std::forward<value>(*this).get_bool();
}
#endif

simdjson_really_inline simdjson_result<array_iterator<value>> value::begin() & noexcept {
  return array_iterator<value>::start(*this, json);
}
simdjson_really_inline simdjson_result<array_iterator<value>> value::end() & noexcept {
  return {};
}

simdjson_really_inline void value::log_value(const char *type) const noexcept {
  char json_char[]{char(json[0]), '\0'};
  logger::log_value(*iter, type, json_char);
}
simdjson_really_inline void value::log_error(const char *message) const noexcept {
  char json_char[]{char(json[0]), '\0'};
  logger::log_error(*iter, message, json_char);
}

//
// For array_iterator
//
simdjson_really_inline json_iterator &value::get_iterator() noexcept {
  return *iter;
}
simdjson_really_inline json_iterator_ref value::borrow_iterator() noexcept {
  return iter.borrow();
}
simdjson_really_inline bool value::is_iterator_alive() const noexcept {
  return iter.is_alive();
}
simdjson_really_inline void value::iteration_finished() noexcept {
  iter.release();
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

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<SIMDJSON_IMPLEMENTATION::ondemand::value>> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::begin() & noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<SIMDJSON_IMPLEMENTATION::ondemand::value>> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::end() & noexcept {
  if (error()) { return error(); }
  return {};
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_array() noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_array();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_object() noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_object();
}
simdjson_really_inline simdjson_result<uint64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_uint64() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_uint64();
}
simdjson_really_inline simdjson_result<uint64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_uint64() & noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_uint64();
}
simdjson_really_inline simdjson_result<int64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_int64() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_int64();
}
simdjson_really_inline simdjson_result<int64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_int64() & noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_int64();
}
simdjson_really_inline simdjson_result<double> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_double() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_double();
}
simdjson_really_inline simdjson_result<double> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_double() & noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_double();
}
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_string() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_string();
}
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_string() & noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_string();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_raw_json_string() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_raw_json_string();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_raw_json_string() & noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_bool() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_bool();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_bool() & noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get_bool();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::is_null() && noexcept {
  if (error()) { return false; }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).is_null();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::is_null() & noexcept {
  if (error()) { return false; }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).is_null();
}

template<typename T> simdjson_really_inline simdjson_result<T> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get() & noexcept {
  if (error()) { return error(); }
  return first.get<T>();
}
template<typename T> simdjson_really_inline simdjson_result<T> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get<T>();
}
template<typename T> simdjson_really_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get(T &out) & noexcept {
  if (error()) { return error(); }
  return first.get<T>(out);
}
template<typename T> simdjson_really_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get(T &out) && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first).get<T>(out);
}

template<> simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get<SIMDJSON_IMPLEMENTATION::ondemand::value>() & noexcept = delete;
template<> simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get<SIMDJSON_IMPLEMENTATION::ondemand::value>() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
template<> simdjson_really_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get<SIMDJSON_IMPLEMENTATION::ondemand::value>(SIMDJSON_IMPLEMENTATION::ondemand::value &out) & noexcept = delete;
template<> simdjson_really_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get<SIMDJSON_IMPLEMENTATION::ondemand::value>(SIMDJSON_IMPLEMENTATION::ondemand::value &out) && noexcept {
  if (error()) { return error(); }
  out = std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
  return SUCCESS;
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_IMPLEMENTATION::ondemand::array() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_IMPLEMENTATION::ondemand::object() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator uint64_t() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator uint64_t() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator int64_t() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator int64_t() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator double() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator double() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator std::string_view() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator std::string_view() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator bool() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator bool() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(first);
}
#endif

} // namespace simdjson
