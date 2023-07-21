#ifndef SIMDJSON_GENERIC_ONDEMAND_SERIALIZATION_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_SERIALIZATION_INL_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/array.h"
#include "simdjson/generic/ondemand/document-inl.h"
#include "simdjson/generic/ondemand/json_type.h"
#include "simdjson/generic/ondemand/object.h"
#include "simdjson/generic/ondemand/serialization.h"
#include "simdjson/generic/ondemand/value.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {

inline std::string_view trim(const std::string_view str) noexcept {
  // We can almost surely do better by rolling our own find_first_not_of function.
  size_t first = str.find_first_not_of(" \t\n\r");
  // If we have the empty string (just white space), then no trimming is possible, and
  // we return the empty string_view.
  if (std::string_view::npos == first) { return std::string_view(); }
  size_t last = str.find_last_not_of(" \t\n\r");
  return str.substr(first, (last - first + 1));
}


inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::document& x) noexcept {
  std::string_view v;
  auto error = x.raw_json().get(v);
  if(error) {return error; }
  return trim(v);
}

inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::document_reference& x) noexcept {
  std::string_view v;
  auto error = x.raw_json().get(v);
  if(error) {return error; }
  return trim(v);
}

inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::value& x) noexcept {
  /**
   * If we somehow receive a value that has already been consumed,
   * then the following code could be in trouble. E.g., we create
   * an array as needed, but if an array was already created, then
   * it could be bad.
   */
  using namespace SIMDJSON_IMPLEMENTATION::ondemand;
  SIMDJSON_IMPLEMENTATION::ondemand::json_type t;
  auto error = x.type().get(t);
  if(error != SUCCESS) { return error; }
  switch (t)
  {
    case json_type::array:
    {
      SIMDJSON_IMPLEMENTATION::ondemand::array array;
      error = x.get_array().get(array);
      if(error) { return error; }
      return to_json_string(array);
    }
    case json_type::object:
    {
      SIMDJSON_IMPLEMENTATION::ondemand::object object;
      error = x.get_object().get(object);
      if(error) { return error; }
      return to_json_string(object);
    }
    default:
      return trim(x.raw_json_token());
  }
}

inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::object& x) noexcept {
  std::string_view v;
  auto error = x.raw_json().get(v);
  if(error) {return error; }
  return trim(v);
}

inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::array& x) noexcept {
  std::string_view v;
  auto error = x.raw_json().get(v);
  if(error) {return error; }
  return trim(v);
}

inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document> x) {
  if (x.error()) { return x.error(); }
  return to_json_string(x.value_unsafe());
}

inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference> x) {
  if (x.error()) { return x.error(); }
  return to_json_string(x.value_unsafe());
}

inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> x) {
  if (x.error()) { return x.error(); }
  return to_json_string(x.value_unsafe());
}

inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> x) {
  if (x.error()) { return x.error(); }
  return to_json_string(x.value_unsafe());
}

inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> x) {
  if (x.error()) { return x.error(); }
  return to_json_string(x.value_unsafe());
}
} // namespace simdjson

namespace simdjson { namespace SIMDJSON_IMPLEMENTATION { namespace ondemand {

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value x) {
  std::string_view v;
  auto error = simdjson::to_json_string(x).get(v);
  if(error == simdjson::SUCCESS) {
    return (out << v);
  } else {
    throw simdjson::simdjson_error(error);
  }
}
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value> x) {
  if (x.error()) { throw simdjson::simdjson_error(x.error()); }
  return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value x) {
  std::string_view v;
  auto error = simdjson::to_json_string(x).get(v);
  if(error == simdjson::SUCCESS) {
    return (out << v);
  } else {
    return (out << error);
  }
}
#endif

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array value) {
  std::string_view v;
  auto error = simdjson::to_json_string(value).get(v);
  if(error == simdjson::SUCCESS) {
    return (out << v);
  } else {
    throw simdjson::simdjson_error(error);
  }
}
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array> x) {
  if (x.error()) { throw simdjson::simdjson_error(x.error()); }
  return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array value) {
  std::string_view v;
  auto error = simdjson::to_json_string(value).get(v);
  if(error == simdjson::SUCCESS) {
    return (out << v);
  } else {
    return (out << error);
  }
}
#endif

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document& value)  {
  std::string_view v;
  auto error = simdjson::to_json_string(value).get(v);
  if(error == simdjson::SUCCESS) {
    return (out << v);
  } else {
    throw simdjson::simdjson_error(error);
  }
}
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document_reference& value)  {
  std::string_view v;
  auto error = simdjson::to_json_string(value).get(v);
  if(error == simdjson::SUCCESS) {
    return (out << v);
  } else {
    throw simdjson::simdjson_error(error);
  }
}
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document>&& x) {
  if (x.error()) { throw simdjson::simdjson_error(x.error()); }
  return (out << x.value());
}
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document_reference>&& x) {
  if (x.error()) { throw simdjson::simdjson_error(x.error()); }
  return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document& value)  {
  std::string_view v;
  auto error = simdjson::to_json_string(value).get(v);
  if(error == simdjson::SUCCESS) {
    return (out << v);
  } else {
    return (out << error);
  }
}
#endif

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object value) {
  std::string_view v;
  auto error = simdjson::to_json_string(value).get(v);
  if(error == simdjson::SUCCESS) {
    return (out << v);
  } else {
    throw simdjson::simdjson_error(error);
  }
}
inline std::ostream& operator<<(std::ostream& out,  simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object> x) {
  if (x.error()) { throw  simdjson::simdjson_error(x.error()); }
  return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object value) {
  std::string_view v;
  auto error = simdjson::to_json_string(value).get(v);
  if(error == simdjson::SUCCESS) {
    return (out << v);
  } else {
    return (out << error);
  }
}
#endif
}}} // namespace simdjson::SIMDJSON_IMPLEMENTATION::ondemand

#endif // SIMDJSON_GENERIC_ONDEMAND_SERIALIZATION_INL_H