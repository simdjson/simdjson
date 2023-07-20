#ifndef SIMDJSON_GENERIC_IMPLEMENTATION_SIMDJSON_RESULT_BASE_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_IMPLEMENTATION_SIMDJSON_RESULT_BASE_INL_H
#include "simdjson/generic/base.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

//
// internal::implementation_simdjson_result_base<T> inline implementation
//

template<typename T>
simdjson_inline void implementation_simdjson_result_base<T>::tie(T &value, error_code &error) && noexcept {
  error = this->second;
  if (!error) {
    value = std::forward<implementation_simdjson_result_base<T>>(*this).first;
  }
}

template<typename T>
simdjson_warn_unused simdjson_inline error_code implementation_simdjson_result_base<T>::get(T &value) && noexcept {
  error_code error;
  std::forward<implementation_simdjson_result_base<T>>(*this).tie(value, error);
  return error;
}

template<typename T>
simdjson_inline error_code implementation_simdjson_result_base<T>::error() const noexcept {
  return this->second;
}

#if SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_inline T& implementation_simdjson_result_base<T>::value() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return this->first;
}

template<typename T>
simdjson_inline T&& implementation_simdjson_result_base<T>::value() && noexcept(false) {
  return std::forward<implementation_simdjson_result_base<T>>(*this).take_value();
}

template<typename T>
simdjson_inline T&& implementation_simdjson_result_base<T>::take_value() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<T>(this->first);
}

template<typename T>
simdjson_inline implementation_simdjson_result_base<T>::operator T&&() && noexcept(false) {
  return std::forward<implementation_simdjson_result_base<T>>(*this).take_value();
}

#endif // SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_inline const T& implementation_simdjson_result_base<T>::value_unsafe() const& noexcept {
  return this->first;
}

template<typename T>
simdjson_inline T& implementation_simdjson_result_base<T>::value_unsafe() & noexcept {
  return this->first;
}

template<typename T>
simdjson_inline T&& implementation_simdjson_result_base<T>::value_unsafe() && noexcept {
  return std::forward<T>(this->first);
}

template<typename T>
simdjson_inline implementation_simdjson_result_base<T>::implementation_simdjson_result_base(T &&value, error_code error) noexcept
    : first{std::forward<T>(value)}, second{error} {}
template<typename T>
simdjson_inline implementation_simdjson_result_base<T>::implementation_simdjson_result_base(error_code error) noexcept
    : implementation_simdjson_result_base(T{}, error) {}
template<typename T>
simdjson_inline implementation_simdjson_result_base<T>::implementation_simdjson_result_base(T &&value) noexcept
    : implementation_simdjson_result_base(std::forward<T>(value), SUCCESS) {}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_IMPLEMENTATION_SIMDJSON_RESULT_BASE_INL_H