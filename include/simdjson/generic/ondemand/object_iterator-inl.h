namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

//
// object_iterator
//

simdjson_really_inline object_iterator::object_iterator(json_iterator_ref &_iter) noexcept : iter{&_iter} {}

simdjson_really_inline simdjson_result<field> object_iterator::operator*() noexcept {
  if ((*iter)->error()) { iter->release(); return (*iter)->error(); }
  return field::start(iter->borrow());
}
simdjson_really_inline bool object_iterator::operator==(const object_iterator &other) noexcept {
  return !(*this != other);
}
simdjson_really_inline bool object_iterator::operator!=(const object_iterator &) noexcept {
  return iter->is_alive();
}
simdjson_really_inline object_iterator &object_iterator::operator++() noexcept {
  if ((*iter)->error()) { return *this; }
  bool has_value;
  error_code error = (*iter)->has_next_field().get(has_value);
  if (!(error || has_value)) { iter->release(); }
  return *this;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::object_iterator &&value
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>(value))
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::simdjson_result(error_code error) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>({}, error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::operator*() noexcept {
  if (error()) { second = SUCCESS; return error(); }
  return *first;
}
// Assumes it's being compared with the end. true if depth < iter->depth.
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &other) noexcept {
  if (error()) { return true; }
  return first == other.first;
}
// Assumes it's being compared with the end. true if depth >= iter->depth.
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &other) noexcept {
  if (error()) { return false; }
  return first != other.first;
}
// Checks for ']' and ','
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator>::operator++() noexcept {
  if (error()) { return *this; }
  ++first;
  return *this;
}

} // namespace simdjson
