#ifndef SIMDJSON_ERROR_INL_H
#define SIMDJSON_ERROR_INL_H

#include "simdjson/error.h"

#include <iostream>

namespace simdjson {

inline bool is_fatal(error_code error) noexcept {
  return error == TAPE_ERROR || error == INCOMPLETE_ARRAY_OR_OBJECT;
}

namespace internal {
  // We store the error code so we can validate the error message is associated with the right code
  struct error_code_info {
    error_code code;
    const char* message; // do not use a fancy std::string where a simple C string will do (no alloc, no destructor)
  };
  // These MUST match the codes in error_code. We check this constraint in basictests.
  extern SIMDJSON_DLLIMPORTEXPORT const error_code_info error_codes[];
} // namespace internal


inline const char *error_message(error_code error) noexcept {
  // If you're using error_code, we're trusting you got it from the enum.
  return internal::error_codes[int(error)].message;
}

// deprecated function
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
inline const std::string error_message(int error) noexcept {
  if (error < 0 || error >= error_code::NUM_ERROR_CODES) {
    return internal::error_codes[UNEXPECTED_ERROR].message;
  }
  return internal::error_codes[error].message;
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API

inline std::ostream& operator<<(std::ostream& out, error_code error) noexcept {
  return out << error_message(error);
}

namespace internal {

//
// internal::simdjson_result_base<T> inline implementation
//

template<typename T>
simdjson_inline void simdjson_result_base<T>::tie(T &value, error_code &error) && noexcept {
  error = this->second;
  if (!error) {
    value = std::forward<simdjson_result_base<T>>(*this).first;
  }
}

template<typename T>
simdjson_warn_unused simdjson_inline error_code simdjson_result_base<T>::get(T &value) && noexcept {
  error_code error;
  std::forward<simdjson_result_base<T>>(*this).tie(value, error);
  return error;
}

template<typename T>
simdjson_inline error_code simdjson_result_base<T>::error() const noexcept {
  return this->second;
}

#if SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_inline T& simdjson_result_base<T>::value() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return this->first;
}

template<typename T>
simdjson_inline T&& simdjson_result_base<T>::value() && noexcept(false) {
  return std::forward<simdjson_result_base<T>>(*this).take_value();
}

template<typename T>
simdjson_inline T&& simdjson_result_base<T>::take_value() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<T>(this->first);
}

template<typename T>
simdjson_inline simdjson_result_base<T>::operator T&&() && noexcept(false) {
  return std::forward<simdjson_result_base<T>>(*this).take_value();
}

#endif // SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_inline const T& simdjson_result_base<T>::value_unsafe() const& noexcept {
  return this->first;
}

template<typename T>
simdjson_inline T&& simdjson_result_base<T>::value_unsafe() && noexcept {
  return std::forward<T>(this->first);
}

template<typename T>
simdjson_inline simdjson_result_base<T>::simdjson_result_base(T &&value, error_code error) noexcept
    : std::pair<T, error_code>(std::forward<T>(value), error) {}
template<typename T>
simdjson_inline simdjson_result_base<T>::simdjson_result_base(error_code error) noexcept
    : simdjson_result_base(T{}, error) {}
template<typename T>
simdjson_inline simdjson_result_base<T>::simdjson_result_base(T &&value) noexcept
    : simdjson_result_base(std::forward<T>(value), SUCCESS) {}
template<typename T>
simdjson_inline simdjson_result_base<T>::simdjson_result_base() noexcept
    : simdjson_result_base(T{}, UNINITIALIZED) {}

} // namespace internal

///
/// simdjson_result<T> inline implementation
///

template<typename T>
simdjson_inline void simdjson_result<T>::tie(T &value, error_code &error) && noexcept {
  std::forward<internal::simdjson_result_base<T>>(*this).tie(value, error);
}

template<typename T>
simdjson_warn_unused simdjson_inline error_code simdjson_result<T>::get(T &value) && noexcept {
  return std::forward<internal::simdjson_result_base<T>>(*this).get(value);
}

template<typename T>
simdjson_inline error_code simdjson_result<T>::error() const noexcept {
  return internal::simdjson_result_base<T>::error();
}

#if SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_inline T& simdjson_result<T>::value() & noexcept(false) {
  return internal::simdjson_result_base<T>::value();
}

template<typename T>
simdjson_inline T&& simdjson_result<T>::value() && noexcept(false) {
  return std::forward<internal::simdjson_result_base<T>>(*this).value();
}

template<typename T>
simdjson_inline T&& simdjson_result<T>::take_value() && noexcept(false) {
  return std::forward<internal::simdjson_result_base<T>>(*this).take_value();
}

template<typename T>
simdjson_inline simdjson_result<T>::operator T&&() && noexcept(false) {
  return std::forward<internal::simdjson_result_base<T>>(*this).take_value();
}

#endif // SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_inline const T& simdjson_result<T>::value_unsafe() const& noexcept {
  return internal::simdjson_result_base<T>::value_unsafe();
}

template<typename T>
simdjson_inline T&& simdjson_result<T>::value_unsafe() && noexcept {
  return std::forward<internal::simdjson_result_base<T>>(*this).value_unsafe();
}

template<typename T>
simdjson_inline simdjson_result<T>::simdjson_result(T &&value, error_code error) noexcept
    : internal::simdjson_result_base<T>(std::forward<T>(value), error) {}
template<typename T>
simdjson_inline simdjson_result<T>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<T>(error) {}
template<typename T>
simdjson_inline simdjson_result<T>::simdjson_result(T &&value) noexcept
    : internal::simdjson_result_base<T>(std::forward<T>(value)) {}
template<typename T>
simdjson_inline simdjson_result<T>::simdjson_result() noexcept
    : internal::simdjson_result_base<T>() {}

} // namespace simdjson

#endif // SIMDJSON_ERROR_INL_H
