#ifndef SIMDJSON_INLINE_ERROR_H
#define SIMDJSON_INLINE_ERROR_H

#include "simdjson/error.h"
#include <string>

namespace simdjson::internal {
  // We store the error code so we can validate the error message is associated with the right code
  struct error_code_info {
    error_code code;
    std::string message;
  };
  // These MUST match the codes in error_code. We check this constraint in basictests.
  inline const error_code_info error_codes[] {
    { SUCCESS, "No error" },
    { SUCCESS_AND_HAS_MORE, "No error and buffer still has more data" },
    { CAPACITY, "This parser can't support a document that big" },
    { MEMALLOC, "Error allocating memory, we're most likely out of memory" },
    { TAPE_ERROR, "Something went wrong while writing to the tape" },
    { DEPTH_ERROR, "The JSON document was too deep (too many nested objects and arrays)" },
    { STRING_ERROR, "Problem while parsing a string" },
    { T_ATOM_ERROR, "Problem while parsing an atom starting with the letter 't'" },
    { F_ATOM_ERROR, "Problem while parsing an atom starting with the letter 'f'" },
    { N_ATOM_ERROR, "Problem while parsing an atom starting with the letter 'n'" },
    { NUMBER_ERROR, "Problem while parsing a number" },
    { UTF8_ERROR, "The input is not valid UTF-8" },
    { UNINITIALIZED, "Uninitialized" },
    { EMPTY, "Empty: no JSON found" },
    { UNESCAPED_CHARS, "Within strings, some characters must be escaped, we found unescaped characters" },
    { UNCLOSED_STRING, "A string is opened, but never closed." },
    { UNSUPPORTED_ARCHITECTURE, "simdjson does not have an implementation supported by this CPU architecture (perhaps it's a non-SIMD CPU?)." },
    { INCORRECT_TYPE, "The JSON element does not have the requested type." },
    { NUMBER_OUT_OF_RANGE, "The JSON number is too large or too small to fit within the requested type." },
    { INDEX_OUT_OF_BOUNDS, "Attempted to access an element of a JSON array that is beyond its length." },
    { NO_SUCH_FIELD, "The JSON field referenced does not exist in this object." },
    { IO_ERROR, "Error reading the file." },
    { INVALID_JSON_POINTER, "Invalid JSON pointer syntax." },
    { INVALID_URI_FRAGMENT, "Invalid URI fragment syntax." },
    { UNEXPECTED_ERROR, "Unexpected error, consider reporting this problem as you may have found a bug in simdjson" }
  }; // error_messages[]
} // namespace simdjson::internal

namespace simdjson {

inline const char *error_message(error_code error) noexcept {
  // If you're using error_code, we're trusting you got it from the enum.
  return internal::error_codes[int(error)].message.c_str();
}

inline const std::string &error_message(int error) noexcept {
  if (error < 0 || error >= error_code::NUM_ERROR_CODES) {
    return internal::error_codes[UNEXPECTED_ERROR].message;
  }
  return internal::error_codes[error].message;
}

inline std::ostream& operator<<(std::ostream& out, error_code error) noexcept {
  return out << error_message(error);
}

namespace internal {

//
// internal::simdjson_result_base<T> inline implementation
//

template<typename T>
really_inline void simdjson_result_base<T>::tie(T &value, error_code &error) && noexcept {
  // on the clang compiler that comes with current macOS (Apple clang version 11.0.0),
  // tie(width, error) = size["w"].get<uint64_t>();
  // fails with "error: no viable overloaded '='""
  value = std::forward<simdjson_result_base<T>>(*this).first;
  error = this->second;
}

template<typename T>
really_inline error_code simdjson_result_base<T>::error() const noexcept {
  return this->second;
}

#if SIMDJSON_EXCEPTIONS

template<typename T>
really_inline T& simdjson_result_base<T>::value() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return this->first;
};

template<typename T>
really_inline T&& simdjson_result_base<T>::take_value() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<T>(this->first);
};

template<typename T>
really_inline simdjson_result_base<T>::operator T&&() && noexcept(false) {
  return std::forward<simdjson_result_base<T>>(*this).take_value();
}

#endif // SIMDJSON_EXCEPTIONS

template<typename T>
really_inline simdjson_result_base<T>::simdjson_result_base(T &&value, error_code error) noexcept
    : std::pair<T, error_code>(std::forward<T>(value), error) {}
template<typename T>
really_inline simdjson_result_base<T>::simdjson_result_base(error_code error) noexcept
    : simdjson_result_base(T{}, error) {}
template<typename T>
really_inline simdjson_result_base<T>::simdjson_result_base(T &&value) noexcept
    : simdjson_result_base(std::forward<T>(value), SUCCESS) {}
template<typename T>
really_inline simdjson_result_base<T>::simdjson_result_base() noexcept
    : simdjson_result_base(T{}, UNINITIALIZED) {}

} // namespace internal

///
/// simdjson_result<T> inline implementation
///

template<typename T>
really_inline void simdjson_result<T>::tie(T &value, error_code &error) && noexcept {
  std::forward<internal::simdjson_result_base<T>>(*this).tie(value, error);
}

template<typename T>
really_inline error_code simdjson_result<T>::error() const noexcept {
  return internal::simdjson_result_base<T>::error();
}

#if SIMDJSON_EXCEPTIONS

template<typename T>
really_inline T& simdjson_result<T>::value() noexcept(false) {
  return internal::simdjson_result_base<T>::value();
}

template<typename T>
really_inline T&& simdjson_result<T>::take_value() && noexcept(false) {
  return std::forward<internal::simdjson_result_base<T>>(*this).take_value();
}

template<typename T>
really_inline simdjson_result<T>::operator T&&() && noexcept(false) {
  return std::forward<internal::simdjson_result_base<T>>(*this).take_value();
}

#endif // SIMDJSON_EXCEPTIONS

template<typename T>
really_inline simdjson_result<T>::simdjson_result(T &&value, error_code error) noexcept
    : internal::simdjson_result_base<T>(std::forward<T>(value), error) {}
template<typename T>
really_inline simdjson_result<T>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<T>(error) {}
template<typename T>
really_inline simdjson_result<T>::simdjson_result(T &&value) noexcept
    : internal::simdjson_result_base<T>(std::forward<T>(value)) {}
template<typename T>
really_inline simdjson_result<T>::simdjson_result() noexcept
    : internal::simdjson_result_base<T>() {}

} // namespace simdjson

#endif // SIMDJSON_INLINE_ERROR_H
