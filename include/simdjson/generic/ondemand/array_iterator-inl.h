namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

template<typename T>
simdjson_really_inline array_iterator<T>::array_iterator(T &_iter) noexcept : iter{&_iter} {}

template<typename T>
simdjson_really_inline simdjson_result<array_iterator<T>> array_iterator<T>::start(T &iter, const uint8_t *json) noexcept {
  bool has_value;
  SIMDJSON_TRY( iter.get_iterator().start_array(json).get(has_value) );
  if (!has_value) { iter.iteration_finished(); }
  return array_iterator<T>(iter);
}
template<typename T>
simdjson_really_inline simdjson_result<value> array_iterator<T>::operator*() noexcept {
  error_code error = iter->get_iterator().error();
  if (error) { iter->iteration_finished(); return error; }
  return value::start(iter->borrow_iterator());
}
template<typename T>
simdjson_really_inline bool array_iterator<T>::operator==(const array_iterator<T> &other) noexcept {
  return !(*this != other);
}
template<typename T>
simdjson_really_inline bool array_iterator<T>::operator!=(const array_iterator<T> &) noexcept {
  return iter->is_iterator_alive();
}
template<typename T>
simdjson_really_inline array_iterator<T> &array_iterator<T>::operator++() noexcept {
  // TODO this is a safety rail ... users should exit loops as soon as they receive an error.
  // Nonetheless, let's see if performance is OK with this if statement--the compiler may give it to us for free.
  if (!iter->is_iterator_alive()) { return *this; } // Iterator will be released if there is an error
  bool has_value;
  error_code error = iter->get_iterator().has_next_element().get(has_value); // If there's an error, has_next stays true.
  if (!(error || has_value)) { iter->iteration_finished(); }
  return *this;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<typename T>
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T> &&value
) noexcept
  : SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>>(value))
{
}
template<typename T>
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>>::simdjson_result(error_code error) noexcept
  : SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>>({}, error)
{
}

template<typename T>
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>>::operator*() noexcept {
  if (this->error()) { this->second = SUCCESS; return this->error(); }
  return *this->first;
}
template<typename T>
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>>::operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>> &other) noexcept {
  if (this->error()) { return true; }
  return this->first == other.first;
}
template<typename T>
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>>::operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>> &other) noexcept {
  if (this->error()) { return false; }
  return this->first != other.first;
}
template<typename T>
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>>::operator++() noexcept {
  if (this->error()) { return *this; }
  ++(this->first);
  return *this;
}

} // namespace simdjson
