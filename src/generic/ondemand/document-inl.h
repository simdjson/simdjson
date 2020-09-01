namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline document::document(document &&other) noexcept = default;
simdjson_really_inline document &document::operator=(document &&other) noexcept = default;
simdjson_really_inline document::document(ondemand::json_iterator &&_iter) noexcept
  : iter(std::forward<json_iterator>(_iter))
{
  logger::log_start_value(iter, "document");
}
simdjson_really_inline document::~document() noexcept {
  if (iter.is_alive()) {
    logger::log_end_value(iter, "document");
  }
}

simdjson_really_inline void document::assert_at_start() const noexcept {
  if (!iter.at_start()) {
    logger::log_error(iter, "Document value can only be used once! ondemand::document is a forward-only input iterator.");
    abort(); // TODO is there anything softer we can do? I'd rather not make this a simdjson_result just for user error.
  }
}
simdjson_really_inline value document::as_value() noexcept {
  assert_at_start();
  return value::start(iter.borrow());
}

simdjson_really_inline simdjson_result<array> document::get_array() & noexcept {
  return as_value().get_array();
}
simdjson_really_inline simdjson_result<object> document::get_object() & noexcept {
  return as_value().get_object();
}
simdjson_really_inline simdjson_result<uint64_t> document::get_uint64() noexcept {
  assert_at_start();
  return iter.get_root_uint64();
}
simdjson_really_inline simdjson_result<int64_t> document::get_int64() noexcept {
  assert_at_start();
  return iter.get_root_int64();
}
simdjson_really_inline simdjson_result<double> document::get_double() noexcept {
  assert_at_start();
  return iter.get_root_double();
}
simdjson_really_inline simdjson_result<std::string_view> document::get_string() & noexcept {
  return as_value().get_string();
}
simdjson_really_inline simdjson_result<raw_json_string> document::get_raw_json_string() & noexcept {
  return as_value().get_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> document::get_bool() noexcept {
  assert_at_start();
  return iter.get_root_bool();
}
simdjson_really_inline bool document::is_null() noexcept {
  assert_at_start();
  return iter.root_is_null();
}


#if SIMDJSON_EXCEPTIONS
simdjson_really_inline document::operator array() & noexcept(false) { return get_array(); }
simdjson_really_inline document::operator object() & noexcept(false) { return get_object(); }
simdjson_really_inline document::operator uint64_t() noexcept(false) { return get_uint64(); }
simdjson_really_inline document::operator int64_t() noexcept(false) { return get_int64(); }
simdjson_really_inline document::operator double() noexcept(false) { return get_double(); }
simdjson_really_inline document::operator std::string_view() & noexcept(false) { return get_string(); }
simdjson_really_inline document::operator raw_json_string() & noexcept(false) { return get_raw_json_string(); }
simdjson_really_inline document::operator bool() noexcept(false) { return get_bool(); }
#endif

simdjson_really_inline simdjson_result<array::iterator> document::begin() & noexcept {
  return get_array().begin();
}
simdjson_really_inline simdjson_result<array::iterator> document::end() & noexcept {
  return {};
}
simdjson_really_inline simdjson_result<value> document::operator[](std::string_view key) & noexcept {
  return get_object()[key];
}
simdjson_really_inline simdjson_result<value> document::operator[](const char *key) & noexcept {
  return get_object()[key];
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::document &&value
) noexcept :
    internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::document>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::simdjson_result(
  error_code error
) noexcept :
    internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document>(
      error
    )
{
}

// TODO make sure the passing of a pointer here isn't about to cause us trouble
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::as_value() noexcept {
  if (error()) { return error(); }
  return first.as_value();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::begin() & noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::end() & noexcept {
  return {};
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator[](std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator[](const char *key) & noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_array() & noexcept {
  if (error()) { return error(); }
  return first.get_array();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_object() & noexcept {
  if (error()) { return error(); }
  return first.get_object();
}
simdjson_really_inline simdjson_result<uint64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_uint64() noexcept {
  if (error()) { return error(); }
  return first.get_uint64();
}
simdjson_really_inline simdjson_result<int64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_int64() noexcept {
  if (error()) { return error(); }
  return first.get_int64();
}
simdjson_really_inline simdjson_result<double> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_double() noexcept {
  if (error()) { return error(); }
  return first.get_double();
}
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_string() & noexcept {
  if (error()) { return error(); }
  return first.get_string();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_raw_json_string() & noexcept {
  if (error()) { return error(); }
  return first.get_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_bool() noexcept {
  if (error()) { return error(); }
  return first.get_bool();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::is_null() noexcept {
  if (error()) { return error(); }
  return first.is_null();
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_IMPLEMENTATION::ondemand::array() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_IMPLEMENTATION::ondemand::object() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator uint64_t() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator int64_t() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator double() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator std::string_view() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator bool() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
#endif

} // namespace simdjson
