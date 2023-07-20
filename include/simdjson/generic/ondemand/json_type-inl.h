#ifndef SIMDJSON_GENERIC_ONDEMAND_JSON_TYPE_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_JSON_TYPE_INL_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/json_type.h"
#include "simdjson/generic/implementation_simdjson_result_base-inl.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

inline std::ostream& operator<<(std::ostream& out, json_type type) noexcept {
    switch (type) {
        case json_type::array: out << "array"; break;
        case json_type::object: out << "object"; break;
        case json_type::number: out << "number"; break;
        case json_type::string: out << "string"; break;
        case json_type::boolean: out << "boolean"; break;
        case json_type::null: out << "null"; break;
        default: SIMDJSON_UNREACHABLE();
    }
    return out;
}

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson_result<json_type> &type) noexcept(false) {
    return out << type.value();
}
#endif



simdjson_inline number_type number::get_number_type() const noexcept {
  return type;
}

simdjson_inline bool number::is_uint64() const noexcept {
  return get_number_type() == number_type::unsigned_integer;
}

simdjson_inline uint64_t number::get_uint64() const noexcept {
  return payload.unsigned_integer;
}

simdjson_inline number::operator uint64_t() const noexcept {
  return get_uint64();
}


simdjson_inline bool number::is_int64() const noexcept {
  return get_number_type() == number_type::signed_integer;
}

simdjson_inline int64_t number::get_int64() const noexcept {
  return payload.signed_integer;
}

simdjson_inline number::operator int64_t() const noexcept {
  return get_int64();
}

simdjson_inline bool number::is_double() const noexcept {
    return get_number_type() == number_type::floating_point_number;
}

simdjson_inline double number::get_double() const noexcept {
  return payload.floating_point_number;
}

simdjson_inline number::operator double() const noexcept {
  return get_double();
}

simdjson_inline double number::as_double() const noexcept {
  if(is_double()) {
    return payload.floating_point_number;
  }
  if(is_int64()) {
    return double(payload.signed_integer);
  }
  return double(payload.unsigned_integer);
}

simdjson_inline void number::append_s64(int64_t value) noexcept {
  payload.signed_integer = value;
  type = number_type::signed_integer;
}

simdjson_inline void number::append_u64(uint64_t value) noexcept {
  payload.unsigned_integer = value;
  type = number_type::unsigned_integer;
}

simdjson_inline void number::append_double(double value) noexcept {
  payload.floating_point_number = value;
  type = number_type::floating_point_number;
}

simdjson_inline void number::skip_double() noexcept {
  type = number_type::floating_point_number;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_type>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_type &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_type>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::json_type>(value)) {}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_type>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_type>(error) {}

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_JSON_TYPE_INL_H