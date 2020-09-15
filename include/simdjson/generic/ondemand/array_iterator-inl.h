namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline array_iterator::array_iterator(json_iterator_ref &_iter) noexcept : iter{&_iter} {}

simdjson_really_inline simdjson_result<value> array_iterator::operator*() noexcept {
  if ((*iter)->error()) { iter->release(); return (*iter)->error(); }
  return value::start(iter->borrow());
}
simdjson_really_inline bool array_iterator::operator==(const array_iterator &other) noexcept {
  return !(*this != other);
}
simdjson_really_inline bool array_iterator::operator!=(const array_iterator &) noexcept {
  return iter->is_alive();
}
simdjson_really_inline array_iterator &array_iterator::operator++() noexcept {
  bool has_value;
  error_code error = (*iter)->has_next_element().get(has_value); // If there's an error, has_next stays true.
  if (!(error || has_value)) { iter->release(); }
  return *this;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::array_iterator &&value
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>(value))
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::simdjson_result(error_code error) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>({}, error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::operator*() noexcept {
  if (error()) { second = SUCCESS; return error(); }
  return *first;
}
// Assumes it's being compared with the end. true if depth < iter->depth.
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> &other) noexcept {
  if (error()) { return true; }
  return first == other.first;
}
// Assumes it's being compared with the end. true if depth >= iter->depth.
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> &other) noexcept {
  if (error()) { return false; }
  return first != other.first;
}
// Checks for ']' and ','
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::operator++() noexcept {
  if (error()) { return *this; }
  ++first;
  return *this;
}

} // namespace simdjson
