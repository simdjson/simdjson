namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline document::document(ondemand::json_iterator &&_iter) noexcept
  : iter{std::forward<json_iterator>(_iter)}
{
  logger::log_start_value(iter, "document");
}

simdjson_really_inline document document::start(json_iterator &&iter) noexcept {
  return document(std::forward<json_iterator>(iter));
}

inline void document::rewind() noexcept {
  iter.rewind();
}

inline std::string document::to_debug_string() noexcept {
  return iter.to_string();
}

inline simdjson_result<const char *> document::current_location() noexcept {
  return iter.current_location();
}

inline bool document::is_alive() noexcept {
  return iter.is_alive();
}
simdjson_really_inline value_iterator document::resume_value_iterator() noexcept {
  return value_iterator(&iter, 1, iter.root_position());
}
simdjson_really_inline value_iterator document::get_root_value_iterator() noexcept {
  return resume_value_iterator();
}
simdjson_really_inline simdjson_result<object> document::start_or_resume_object() noexcept {
  if (iter.at_root()) {
    return get_object();
  } else {
    return object::resume(resume_value_iterator());
  }
}
simdjson_really_inline simdjson_result<value> document::get_value() noexcept {
  // Make sure we start any arrays or objects before returning, so that start_root_<object/array>()
  // gets called.
  iter.assert_at_document_depth();
  switch (*iter.peek()) {
    case '[':
    case '{':
      return value(get_root_value_iterator());
    default:
      // Unfortunately, scalar documents are a special case in simdjson and they cannot
      // be safely converted to value instances.
      return SCALAR_DOCUMENT_AS_VALUE;
      // return value(get_root_value_iterator());
  }
}
simdjson_really_inline simdjson_result<array> document::get_array() & noexcept {
  auto value = get_root_value_iterator();
  return array::start_root(value);
}
simdjson_really_inline simdjson_result<object> document::get_object() & noexcept {
  auto value = get_root_value_iterator();
  return object::start_root(value);
}
simdjson_really_inline simdjson_result<uint64_t> document::get_uint64() noexcept {
  return get_root_value_iterator().get_root_uint64();
}
simdjson_really_inline simdjson_result<uint64_t> document::get_uint64_in_string() noexcept {
  return get_root_value_iterator().get_root_uint64_in_string();
}
simdjson_really_inline simdjson_result<int64_t> document::get_int64() noexcept {
  return get_root_value_iterator().get_root_int64();
}
simdjson_really_inline simdjson_result<int64_t> document::get_int64_in_string() noexcept {
  return get_root_value_iterator().get_root_int64_in_string();
}
simdjson_really_inline simdjson_result<double> document::get_double() noexcept {
  return get_root_value_iterator().get_root_double();
}
simdjson_really_inline simdjson_result<double> document::get_double_in_string() noexcept {
  return get_root_value_iterator().get_root_double_in_string();
}
simdjson_really_inline simdjson_result<std::string_view> document::get_string() noexcept {
  return get_root_value_iterator().get_root_string();
}
simdjson_really_inline simdjson_result<raw_json_string> document::get_raw_json_string() noexcept {
  return get_root_value_iterator().get_root_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> document::get_bool() noexcept {
  return get_root_value_iterator().get_root_bool();
}
simdjson_really_inline bool document::is_null() noexcept {
  return get_root_value_iterator().is_root_null();
}

template<> simdjson_really_inline simdjson_result<array> document::get() & noexcept { return get_array(); }
template<> simdjson_really_inline simdjson_result<object> document::get() & noexcept { return get_object(); }
template<> simdjson_really_inline simdjson_result<raw_json_string> document::get() & noexcept { return get_raw_json_string(); }
template<> simdjson_really_inline simdjson_result<std::string_view> document::get() & noexcept { return get_string(); }
template<> simdjson_really_inline simdjson_result<double> document::get() & noexcept { return get_double(); }
template<> simdjson_really_inline simdjson_result<uint64_t> document::get() & noexcept { return get_uint64(); }
template<> simdjson_really_inline simdjson_result<int64_t> document::get() & noexcept { return get_int64(); }
template<> simdjson_really_inline simdjson_result<bool> document::get() & noexcept { return get_bool(); }
template<> simdjson_really_inline simdjson_result<value> document::get() & noexcept { return get_value(); }

template<> simdjson_really_inline simdjson_result<raw_json_string> document::get() && noexcept { return get_raw_json_string(); }
template<> simdjson_really_inline simdjson_result<std::string_view> document::get() && noexcept { return get_string(); }
template<> simdjson_really_inline simdjson_result<double> document::get() && noexcept { return std::forward<document>(*this).get_double(); }
template<> simdjson_really_inline simdjson_result<uint64_t> document::get() && noexcept { return std::forward<document>(*this).get_uint64(); }
template<> simdjson_really_inline simdjson_result<int64_t> document::get() && noexcept { return std::forward<document>(*this).get_int64(); }
template<> simdjson_really_inline simdjson_result<bool> document::get() && noexcept { return std::forward<document>(*this).get_bool(); }
template<> simdjson_really_inline simdjson_result<value> document::get() && noexcept { return get_value(); }

template<typename T> simdjson_really_inline error_code document::get(T &out) & noexcept {
  return get<T>().get(out);
}
template<typename T> simdjson_really_inline error_code document::get(T &out) && noexcept {
  return std::forward<document>(*this).get<T>().get(out);
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline document::operator array() & noexcept(false) { return get_array(); }
simdjson_really_inline document::operator object() & noexcept(false) { return get_object(); }
simdjson_really_inline document::operator uint64_t() noexcept(false) { return get_uint64(); }
simdjson_really_inline document::operator int64_t() noexcept(false) { return get_int64(); }
simdjson_really_inline document::operator double() noexcept(false) { return get_double(); }
simdjson_really_inline document::operator std::string_view() noexcept(false) { return get_string(); }
simdjson_really_inline document::operator raw_json_string() noexcept(false) { return get_raw_json_string(); }
simdjson_really_inline document::operator bool() noexcept(false) { return get_bool(); }
simdjson_really_inline document::operator value() noexcept(false) { return get_value(); }

#endif
simdjson_really_inline simdjson_result<size_t> document::count_elements() & noexcept {
  auto a = get_array();
  simdjson_result<size_t> answer = a.count_elements();
  /* If there was an array, we are now left pointing at its first element. */
  if(answer.error() == SUCCESS) {
    iter._depth = 1 ; /* undoing the increment so we go back at the doc depth.*/
    iter.assert_at_document_depth();
  }
  return answer;
}
simdjson_really_inline simdjson_result<size_t> document::count_fields() & noexcept {
  auto a = get_object();
  simdjson_result<size_t> answer = a.count_fields();
  /* If there was an array, we are now left pointing at its first element. */
  if(answer.error() == SUCCESS) {
    iter._depth = 1 ; /* undoing the increment so we go back at the doc depth.*/
    iter.assert_at_document_depth();
  }
  return answer;
}
simdjson_really_inline simdjson_result<value> document::at(size_t index) & noexcept {
  auto a = get_array();
  return a.at(index);
}
simdjson_really_inline simdjson_result<array_iterator> document::begin() & noexcept {
  return get_array().begin();
}
simdjson_really_inline simdjson_result<array_iterator> document::end() & noexcept {
  return {};
}

simdjson_really_inline simdjson_result<value> document::find_field(std::string_view key) & noexcept {
  return start_or_resume_object().find_field(key);
}
simdjson_really_inline simdjson_result<value> document::find_field(const char *key) & noexcept {
  return start_or_resume_object().find_field(key);
}
simdjson_really_inline simdjson_result<value> document::find_field_unordered(std::string_view key) & noexcept {
  return start_or_resume_object().find_field_unordered(key);
}
simdjson_really_inline simdjson_result<value> document::find_field_unordered(const char *key) & noexcept {
  return start_or_resume_object().find_field_unordered(key);
}
simdjson_really_inline simdjson_result<value> document::operator[](std::string_view key) & noexcept {
  return start_or_resume_object()[key];
}
simdjson_really_inline simdjson_result<value> document::operator[](const char *key) & noexcept {
  return start_or_resume_object()[key];
}

simdjson_really_inline error_code document::consume() noexcept {
  auto error = iter.skip_child(0);
  if(error) { iter.abandon(); }
  return error;
}

simdjson_really_inline simdjson_result<std::string_view> document::raw_json() noexcept {
  auto _iter = get_root_value_iterator();
  const uint8_t * starting_point{_iter.peek_start()};
  auto error = consume();
  if(error) { return error; }
  // After 'consume()', we could be left pointing just beyond the document, but that
  // is ok because we are not going to dereference the final pointer position, we just
  // use it to compute the length in bytes.
  const uint8_t * final_point{iter.unsafe_pointer()};
  return std::string_view(reinterpret_cast<const char*>(starting_point), size_t(final_point - starting_point));
}

simdjson_really_inline simdjson_result<json_type> document::type() noexcept {
  return get_root_value_iterator().type();
}

simdjson_really_inline simdjson_result<bool> document::is_scalar() noexcept {
  json_type this_type;
  auto error = type().get(this_type);
  if(error) { return error; }
  return ! ((this_type == json_type::array) || (this_type == json_type::object));
}

simdjson_really_inline bool document::is_negative() noexcept {
  return get_root_value_iterator().is_root_negative();
}

simdjson_really_inline simdjson_result<bool> document::is_integer() noexcept {
  return get_root_value_iterator().is_root_integer();
}

simdjson_really_inline simdjson_result<number_type> document::get_number_type() noexcept {
  return get_root_value_iterator().get_root_number_type();
}

simdjson_really_inline simdjson_result<number> document::get_number() noexcept {
  return get_root_value_iterator().get_root_number();
}


simdjson_really_inline simdjson_result<std::string_view> document::raw_json_token() noexcept {
  auto _iter = get_root_value_iterator();
  return std::string_view(reinterpret_cast<const char*>(_iter.peek_start()), _iter.peek_start_length());
}

simdjson_really_inline simdjson_result<value> document::at_pointer(std::string_view json_pointer) noexcept {
  rewind(); // Rewind the document each time at_pointer is called
  if (json_pointer.empty()) {
    return this->get_value();
  }
  json_type t;
  SIMDJSON_TRY(type().get(t));
  switch (t)
  {
    case json_type::array:
      return (*this).get_array().at_pointer(json_pointer);
    case json_type::object:
      return (*this).get_object().at_pointer(json_pointer);
    default:
      return INVALID_JSON_POINTER;
  }
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::document &&value
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::document>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::simdjson_result(
  error_code error
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document>(
      error
    )
{
}
simdjson_really_inline simdjson_result<size_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::count_elements() & noexcept {
  if (error()) { return error(); }
  return first.count_elements();
}
simdjson_really_inline simdjson_result<size_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::count_fields() & noexcept {
  if (error()) { return error(); }
  return first.count_fields();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::at(size_t index) & noexcept {
  if (error()) { return error(); }
  return first.at(index);
}
simdjson_really_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::rewind() noexcept {
  if (error()) { return error(); }
  first.rewind();
  return SUCCESS;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::begin() & noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::end() & noexcept {
  return {};
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::find_field_unordered(std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first.find_field_unordered(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::find_field_unordered(const char *key) & noexcept {
  if (error()) { return error(); }
  return first.find_field_unordered(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator[](std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator[](const char *key) & noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::find_field(std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first.find_field(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::find_field(const char *key) & noexcept {
  if (error()) { return error(); }
  return first.find_field(key);
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
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_string() noexcept {
  if (error()) { return error(); }
  return first.get_string();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_raw_json_string() noexcept {
  if (error()) { return error(); }
  return first.get_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_bool() noexcept {
  if (error()) { return error(); }
  return first.get_bool();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_value() noexcept {
  if (error()) { return error(); }
  return first.get_value();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::is_null() noexcept {
  if (error()) { return error(); }
  return first.is_null();
}

template<typename T>
simdjson_really_inline simdjson_result<T> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get() & noexcept {
  if (error()) { return error(); }
  return first.get<T>();
}
template<typename T>
simdjson_really_inline simdjson_result<T> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::document>(first).get<T>();
}
template<typename T>
simdjson_really_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get(T &out) & noexcept {
  if (error()) { return error(); }
  return first.get<T>(out);
}
template<typename T>
simdjson_really_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get(T &out) && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::document>(first).get<T>(out);
}

template<> simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get<SIMDJSON_IMPLEMENTATION::ondemand::document>() & noexcept = delete;
template<> simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get<SIMDJSON_IMPLEMENTATION::ondemand::document>() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_IMPLEMENTATION::ondemand::document>(first);
}
template<> simdjson_really_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get<SIMDJSON_IMPLEMENTATION::ondemand::document>(SIMDJSON_IMPLEMENTATION::ondemand::document &out) & noexcept = delete;
template<> simdjson_really_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get<SIMDJSON_IMPLEMENTATION::ondemand::document>(SIMDJSON_IMPLEMENTATION::ondemand::document &out) && noexcept {
  if (error()) { return error(); }
  out = std::forward<SIMDJSON_IMPLEMENTATION::ondemand::document>(first);
  return SUCCESS;
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_type> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::type() noexcept {
  if (error()) { return error(); }
  return first.type();
}

simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::is_scalar() noexcept {
  if (error()) { return error(); }
  return first.is_scalar();
}


simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::is_negative() noexcept {
  if (error()) { return error(); }
  return first.is_negative();
}

simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::is_integer() noexcept {
  if (error()) { return error(); }
  return first.is_integer();
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::number_type> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_number_type() noexcept {
  if (error()) { return error(); }
  return first.get_number_type();
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::number> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::get_number() noexcept {
  if (error()) { return error(); }
  return first.get_number();
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
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator std::string_view() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator bool() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_IMPLEMENTATION::ondemand::value() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
#endif


simdjson_really_inline simdjson_result<const char *> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::current_location() noexcept {
  if (error()) { return error(); }
  return first.current_location();
}

simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::raw_json_token() noexcept {
  if (error()) { return error(); }
  return first.raw_json_token();
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document>::at_pointer(std::string_view json_pointer) noexcept {
  if (error()) { return error(); }
  return first.at_pointer(json_pointer);
}


} // namespace simdjson


namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline document_reference::document_reference() noexcept : doc{nullptr} {}
simdjson_really_inline document_reference::document_reference(document &d) noexcept : doc(&d) {}
simdjson_really_inline void document_reference::rewind() noexcept { doc->rewind(); }
simdjson_really_inline simdjson_result<array> document_reference::get_array() & noexcept { return doc->get_array(); }
simdjson_really_inline simdjson_result<object> document_reference::get_object() & noexcept { return doc->get_object(); }
simdjson_really_inline simdjson_result<uint64_t> document_reference::get_uint64() noexcept { return doc->get_uint64(); }
simdjson_really_inline simdjson_result<int64_t> document_reference::get_int64() noexcept { return doc->get_int64(); }
simdjson_really_inline simdjson_result<double> document_reference::get_double() noexcept { return doc->get_double(); }
simdjson_really_inline simdjson_result<std::string_view> document_reference::get_string() noexcept { return doc->get_string(); }
simdjson_really_inline simdjson_result<raw_json_string> document_reference::get_raw_json_string() noexcept { return doc->get_raw_json_string(); }
simdjson_really_inline simdjson_result<bool> document_reference::get_bool() noexcept { return doc->get_bool(); }
simdjson_really_inline simdjson_result<value> document_reference::get_value() noexcept { return doc->get_value(); }
simdjson_really_inline bool document_reference::is_null() noexcept { return doc->is_null(); }

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline document_reference::operator array() & noexcept(false) { return array(*doc); }
simdjson_really_inline document_reference::operator object() & noexcept(false) { return object(*doc); }
simdjson_really_inline document_reference::operator uint64_t() noexcept(false) { return uint64_t(*doc); }
simdjson_really_inline document_reference::operator int64_t() noexcept(false) { return int64_t(*doc); }
simdjson_really_inline document_reference::operator double() noexcept(false) { return double(*doc); }
simdjson_really_inline document_reference::operator std::string_view() noexcept(false) { return std::string_view(*doc); }
simdjson_really_inline document_reference::operator raw_json_string() noexcept(false) { return raw_json_string(*doc); }
simdjson_really_inline document_reference::operator bool() noexcept(false) { return bool(*doc); }
simdjson_really_inline document_reference::operator value() noexcept(false) { return value(*doc); }
#endif
simdjson_really_inline simdjson_result<size_t> document_reference::count_elements() & noexcept { return doc->count_elements(); }
simdjson_really_inline simdjson_result<size_t> document_reference::count_fields() & noexcept { return doc->count_fields(); }
simdjson_really_inline simdjson_result<value> document_reference::at(size_t index) & noexcept { return doc->at(index); }
simdjson_really_inline simdjson_result<array_iterator> document_reference::begin() & noexcept { return doc->begin(); }
simdjson_really_inline simdjson_result<array_iterator> document_reference::end() & noexcept { return doc->end(); }
simdjson_really_inline simdjson_result<value> document_reference::find_field(std::string_view key) & noexcept { return doc->find_field(key); }
simdjson_really_inline simdjson_result<value> document_reference::find_field(const char *key) & noexcept { return doc->find_field(key); }
simdjson_really_inline simdjson_result<value> document_reference::operator[](std::string_view key) & noexcept { return (*doc)[key]; }
simdjson_really_inline simdjson_result<value> document_reference::operator[](const char *key) & noexcept { return (*doc)[key]; }
simdjson_really_inline simdjson_result<value> document_reference::find_field_unordered(std::string_view key) & noexcept { return doc->find_field_unordered(key); }
simdjson_really_inline simdjson_result<value> document_reference::find_field_unordered(const char *key) & noexcept { return doc->find_field_unordered(key); }
simdjson_really_inline simdjson_result<json_type> document_reference::type() noexcept { return doc->type(); }
simdjson_really_inline simdjson_result<bool> document_reference::is_scalar() noexcept { return doc->is_scalar(); }
simdjson_really_inline simdjson_result<const char *> document_reference::current_location() noexcept { return doc->current_location(); }
simdjson_really_inline bool document_reference::is_negative() noexcept { return doc->is_negative(); }
simdjson_really_inline simdjson_result<bool> document_reference::is_integer() noexcept { return doc->is_integer(); }
simdjson_really_inline simdjson_result<number_type> document_reference::get_number_type() noexcept { return doc->get_number_type(); }
simdjson_really_inline simdjson_result<number> document_reference::get_number() noexcept { return doc->get_number(); }
simdjson_really_inline simdjson_result<std::string_view> document_reference::raw_json_token() noexcept { return doc->raw_json_token(); }
simdjson_really_inline simdjson_result<value> document_reference::at_pointer(std::string_view json_pointer) noexcept { return doc->at_pointer(json_pointer); }
simdjson_really_inline simdjson_result<std::string_view> document_reference::raw_json() noexcept { return doc->raw_json();}
simdjson_really_inline document_reference::operator document&() const noexcept { return *doc; }

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson



namespace simdjson {
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::document_reference value, error_code error)
  noexcept : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>(value), error) {}


simdjson_really_inline simdjson_result<size_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::count_elements() & noexcept {
  if (error()) { return error(); }
  return first.count_elements();
}
simdjson_really_inline simdjson_result<size_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::count_fields() & noexcept {
  if (error()) { return error(); }
  return first.count_fields();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::at(size_t index) & noexcept {
  if (error()) { return error(); }
  return first.at(index);
}
simdjson_really_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::rewind() noexcept {
  if (error()) { return error(); }
  first.rewind();
  return SUCCESS;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::begin() & noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::end() & noexcept {
  return {};
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::find_field_unordered(std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first.find_field_unordered(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::find_field_unordered(const char *key) & noexcept {
  if (error()) { return error(); }
  return first.find_field_unordered(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator[](std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator[](const char *key) & noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::find_field(std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first.find_field(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::find_field(const char *key) & noexcept {
  if (error()) { return error(); }
  return first.find_field(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_array() & noexcept {
  if (error()) { return error(); }
  return first.get_array();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_object() & noexcept {
  if (error()) { return error(); }
  return first.get_object();
}
simdjson_really_inline simdjson_result<uint64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_uint64() noexcept {
  if (error()) { return error(); }
  return first.get_uint64();
}
simdjson_really_inline simdjson_result<int64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_int64() noexcept {
  if (error()) { return error(); }
  return first.get_int64();
}
simdjson_really_inline simdjson_result<double> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_double() noexcept {
  if (error()) { return error(); }
  return first.get_double();
}
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_string() noexcept {
  if (error()) { return error(); }
  return first.get_string();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_raw_json_string() noexcept {
  if (error()) { return error(); }
  return first.get_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_bool() noexcept {
  if (error()) { return error(); }
  return first.get_bool();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_value() noexcept {
  if (error()) { return error(); }
  return first.get_value();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::is_null() noexcept {
  if (error()) { return error(); }
  return first.is_null();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_type> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::type() noexcept {
  if (error()) { return error(); }
  return first.type();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::is_scalar() noexcept {
  if (error()) { return error(); }
  return first.is_scalar();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::is_negative() noexcept {
  if (error()) { return error(); }
  return first.is_negative();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::is_integer() noexcept {
  if (error()) { return error(); }
  return first.is_integer();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::number_type> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_number_type() noexcept {
  if (error()) { return error(); }
  return first.get_number_type();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::number> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::get_number() noexcept {
  if (error()) { return error(); }
  return first.get_number();
}
#if SIMDJSON_EXCEPTIONS
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator SIMDJSON_IMPLEMENTATION::ondemand::array() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator SIMDJSON_IMPLEMENTATION::ondemand::object() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator uint64_t() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator int64_t() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator double() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator std::string_view() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator bool() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::operator SIMDJSON_IMPLEMENTATION::ondemand::value() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
#endif

simdjson_really_inline simdjson_result<const char *> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::current_location() noexcept {
  if (error()) { return error(); }
  return first.current_location();
}

simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::raw_json_token() noexcept {
  if (error()) { return error(); }
  return first.raw_json_token();
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference>::at_pointer(std::string_view json_pointer) noexcept {
  if (error()) { return error(); }
  return first.at_pointer(json_pointer);
}


} // namespace simdjson
