/**
 * This file is part of the builder API. It is temporarily in the ondemand directory
 * but we will move it to a builder directory later.
 */
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
    /***
      char *padded_buffer = new (std::nothrow) char[totalpaddedlength];
  if (padded_buffer == nullptr) {
    return nullptr;
  }
     */
simdjson_inline bool string_builder::capacity_check(size_t upcoming_bytes) {
  if (upcoming_bytes <= capacity - position) { return true; }
  // check for overflow:
  if (position + upcoming_bytes < position) { return false; }
  grow_buffer(std::max(capacity * 2, position + upcoming_bytes));
  return is_valid;
}

simdjson_inline void string_builder::grow_buffer(size_t desired_capacity) {
  if (!is_valid) { return; }
  std::unique_ptr<char[]> new_buffer(new (std::nothrow) char[desired_capacity]);
  if (new_buffer.get() == nullptr) {
    is_valid = false;
    capacity = 0;
    return;
  }
  memcpy(new_buffer.get(), buffer.get(), position);
  buffer.swap(new_buffer);
  capacity = desired_capacity;
}

simdjson_inline size_t string_builder::size() const {
  return is_valid ? position : 0;
}

} // namespace builder
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_BUILDER_INL_H