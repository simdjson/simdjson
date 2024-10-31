/**
 * This file is part of the builder API. It is temporarily in the ondemand directory
 * but we will move it to a builder directory later.
 */
#include <type_traits>
#ifndef SIMDJSON_GENERIC_BUILDER_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_BUILDER_INL_H
#include "simdjson/generic/builder/json_string_builder.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace builder {

simdjson_inline string_builder::string_builder(size_t initial_capacity) :
  buffer(new (std::nothrow) char[initial_capacity]),
  position(0), capacity(buffer.get() != nullptr ? initial_capacity : 0),
  is_valid(buffer.get() != nullptr) {}

simdjson_inline bool string_builder::capacity_check(size_t upcoming_bytes) {
  // We use the convention that when is_valid is false, then the capacity and
  // the position are 0.
  // Most of the time, this function will return true.
  if (simdjson_likely(upcoming_bytes <= capacity - position)) { return true; }
  // check for overflow, most of the time there is no overflow
  if (simdjson_likely(position + upcoming_bytes < position)) { return false; }
  grow_buffer(std::max(capacity * 2, position + upcoming_bytes));
  return is_valid;
}

simdjson_inline void string_builder::grow_buffer(size_t desired_capacity) {
  if (!is_valid) { return; }
  std::unique_ptr<char[]> new_buffer(new (std::nothrow) char[desired_capacity]);
  if (new_buffer.get() == nullptr) {
    set_valid(false);
    return;
  }
  memcpy(new_buffer.get(), buffer.get(), position);
  buffer.swap(new_buffer);
  capacity = desired_capacity;
}

simdjson_inline void string_builder::set_valid(bool valid) noexcept {
  if(!valid) {
    is_valid = false;
    capacity = 0;
    position = 0;
  } else {
    is_valid = true;
  }
}

simdjson_inline size_t string_builder::size() const {
  return position;
}


template<typename number_type,
  typename = typename std::enable_if<std::is_arithmetic<number_type>::value>::type>
simdjson_inline void string_builder::append(number_type v) noexcept {

}


simdjson_inline void string_builder::append(char c)  noexcept {

}


simdjson_inline void string_builder::append_null()  noexcept {

}

simdjson_inline void clear()  noexcept {
  position = 0;
  // if it was invalid, we should try to repair it
  if(!is_valid) {
    capacity = 0;
    is_valid = true;
  }
}

  /**
   * Append the std::string_view, after escaping it.
   * There is no UTF-8 validation.
   */
  simdjson_inline void escape_and_append(std::string_view input)  noexcept;

  /**
   * Append the std::string_view surrounded by double quotes, after escaping it.
   * There is no UTF-8 validation.
   */
  simdjson_inline void escape_and_append_with_quotes(std::string_view input)  noexcept;

  /**
   * Append the C string directly, without escaping.
   * There is no UTF-8 validation.
   */
  simdjson_inline void append_raw(const char *c)  noexcept;

  /**
   * Append the std::string_view directly, without escaping.
   * There is no UTF-8 validation.
   */
  simdjson_inline void append_raw(std::string_view str)  noexcept;

} // namespace builder
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_BUILDER_INL_H