namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline document::document(document &&other) noexcept :
  iter{std::forward<token_iterator>(other.iter)},
  parser{other.parser}
{
  if (!at_start()) { logger::log_error(iter, "Cannot move document after it has been used"); abort(); }
  other.parser = nullptr;
}
simdjson_really_inline document &document::operator=(document &&other) noexcept {
  iter = std::forward<token_iterator>(other.iter);
  parser = other.parser;
  if (!at_start()) { logger::log_error(iter, "Cannot move document after it has been used"); abort(); }
  other.parser = nullptr;
  return *this;
}

simdjson_really_inline document::document(ondemand::parser *_parser) noexcept
  : iter(_parser->dom_parser.buf, _parser->dom_parser.structural_indexes.get(), 0), parser{_parser}
{
  logger::log_headers();
  parser->current_string_buf_loc = parser->string_buf.get();
  logger::log_start_value(iter, "document");
}
simdjson_really_inline document::~document() noexcept {
  // Release the string buf so it can be reused by the next document
  if (parser) {
    logger::log_end_value(iter, "document");
    parser->current_string_buf_loc = nullptr;
  }
}

simdjson_really_inline value document::as_value() noexcept {
  
  if (!at_start()) {
    logger::log_error(iter, "Document value can only be used once! ondemand::document is a forward-only input iterator.");
    abort(); // TODO is there anything softer we can do? I'd rather not make this a simdjson_result just for user error.
  }
  return value::start(this);
}
simdjson_really_inline bool document::at_start() const noexcept { return iter.index == parser->dom_parser.structural_indexes.get(); }

simdjson_really_inline simdjson_result<array> document::get_array() & noexcept { return as_value().get_array(); }
simdjson_really_inline simdjson_result<object> document::get_object() & noexcept { return as_value().get_object(); }
simdjson_really_inline simdjson_result<uint64_t> document::get_uint64() noexcept { return as_value().get_uint64(); }
simdjson_really_inline simdjson_result<int64_t> document::get_int64() noexcept { return as_value().get_int64(); }
simdjson_really_inline simdjson_result<double> document::get_double() noexcept { return as_value().get_double(); }
simdjson_really_inline simdjson_result<std::string_view> document::get_string() & noexcept { return as_value().get_string(); }
simdjson_really_inline simdjson_result<raw_json_string> document::get_raw_json_string() & noexcept { return as_value().get_raw_json_string(); }
simdjson_really_inline simdjson_result<bool> document::get_bool() noexcept { return as_value().get_bool(); }
simdjson_really_inline bool document::is_null() noexcept { return as_value().is_null(); }

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline document::operator array() & noexcept(false) { return as_value(); }
simdjson_really_inline document::operator object() & noexcept(false) { return as_value(); }
simdjson_really_inline document::operator uint64_t() noexcept(false) { return as_value(); }
simdjson_really_inline document::operator int64_t() noexcept(false) { return as_value(); }
simdjson_really_inline document::operator double() noexcept(false) { return as_value(); }
simdjson_really_inline document::operator std::string_view() & noexcept(false) { return as_value(); }
simdjson_really_inline document::operator raw_json_string() & noexcept(false) { return as_value(); }
simdjson_really_inline document::operator bool() noexcept(false) { return as_value(); }
#endif

simdjson_really_inline array document::begin() & noexcept { return as_value().begin(); }
simdjson_really_inline array document::end() & noexcept { return {}; }
simdjson_really_inline simdjson_result<value> document::operator[](std::string_view key) & noexcept { return as_value()[key]; }

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
  SIMDJSON_IMPLEMENTATION::ondemand::document &&value,
  error_code error
) noexcept :
    internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::document>(value),
      error
    )
{
}

// TODO make sure the passing of a pointer here isn't about to cause us trouble
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::as_value() noexcept {
  if (error()) { return { &first, error() }; }
  return first.as_value();
}
simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::array simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::begin() & noexcept { return as_value().begin(); }
simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::array simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::end() & noexcept { return {}; }
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator[](std::string_view key) & noexcept {
  return as_value()[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator[](const char *key) & noexcept {
  return as_value()[key];
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_array() & noexcept { return as_value().get_array(); }
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_object() & noexcept { return as_value().get_object(); }
simdjson_really_inline simdjson_result<uint64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_uint64() noexcept { return as_value().get_uint64(); }
simdjson_really_inline simdjson_result<int64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_int64() noexcept { return as_value().get_int64(); }
simdjson_really_inline simdjson_result<double> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_double() noexcept { return as_value().get_double(); }
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_string() & noexcept { return as_value().get_string(); }
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_raw_json_string() & noexcept { return as_value().get_raw_json_string(); }
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_bool() noexcept { return as_value().get_bool(); }
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::is_null() noexcept { return as_value().is_null(); }

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_IMPLEMENTATION::ondemand::array() & noexcept(false) { return as_value(); }
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_IMPLEMENTATION::ondemand::object() & noexcept(false) { return as_value(); }
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator uint64_t() noexcept(false) { return as_value(); }
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator int64_t() noexcept(false) { return as_value(); }
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator double() noexcept(false) { return as_value(); }
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator std::string_view() & noexcept(false) { return as_value(); }
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() & noexcept(false) { return as_value(); }
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator bool() noexcept(false) { return as_value(); }
#endif

} // namespace simdjson
