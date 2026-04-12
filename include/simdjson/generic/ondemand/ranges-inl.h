#ifndef SIMDJSON_GENERIC_ONDEMAND_RANGES_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_RANGES_INL_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/ranges.h"
#include "simdjson/generic/ondemand/array-inl.h"
#include "simdjson/generic/ondemand/array_iterator-inl.h"
#include "simdjson/generic/ondemand/object-inl.h"
#include "simdjson/generic/ondemand/object_iterator-inl.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_SUPPORTS_RANGES

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

//
// array_range_iterator
//

simdjson_inline array_range_iterator::array_range_iterator(array_iterator iter) noexcept
  : iter_{iter} {}

simdjson_inline simdjson_result<value> array_range_iterator::operator*() const noexcept {
  return *iter_;
}

simdjson_inline array_range_iterator& array_range_iterator::operator++() noexcept {
  ++iter_;
  return *this;
}

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
simdjson_inline void array_range_iterator::operator++(int) noexcept {
  ++*this;
}
SIMDJSON_POP_DISABLE_WARNINGS

//
// array_range
//

simdjson_inline array_range::array_range(array& arr) noexcept {
  auto b = arr.begin();
  if (b.error()) { error_ = b.error(); return; }
  begin_ = b.value_unsafe();
  end_ = arr.end().value_unsafe();
}

simdjson_inline array_range_iterator array_range::begin() noexcept {
  return array_range_iterator(begin_);
}

simdjson_inline array_range_iterator array_range::end() noexcept {
  return array_range_iterator(end_);
}

//
// object_range_iterator
//

simdjson_inline object_range_iterator::object_range_iterator(object_iterator iter) noexcept
  : iter_{iter} {}

simdjson_inline simdjson_result<field> object_range_iterator::operator*() const noexcept {
  return *iter_;
}

simdjson_inline object_range_iterator& object_range_iterator::operator++() noexcept {
  ++iter_;
  return *this;
}

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
simdjson_inline void object_range_iterator::operator++(int) noexcept {
  ++*this;
}
SIMDJSON_POP_DISABLE_WARNINGS


//
// object_range
//

simdjson_inline object_range::object_range(object& obj) noexcept {
  auto b = obj.begin();
  if (b.error()) { error_ = b.error(); return; }
  begin_ = b.value_unsafe();
  end_ = obj.end().value_unsafe();
}

simdjson_inline object_range_iterator object_range::begin() noexcept {
  return object_range_iterator(begin_);
}

simdjson_inline object_range_iterator object_range::end() noexcept {
  return object_range_iterator(end_);
}

//
// Free functions
//

simdjson_inline array_range get_range(array& arr) noexcept {
  return array_range(arr);
}

simdjson_inline object_range get_key_value_range(object& obj) noexcept {
  return object_range(obj);
}

#if SIMDJSON_EXCEPTIONS
simdjson_inline array_range get_range(simdjson_result<array> result) {
  return array_range(result.value());
}

simdjson_inline object_range get_key_value_range(simdjson_result<object> result) {
  return object_range(result.value());
}
#endif // SIMDJSON_EXCEPTIONS

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

// Verify the range wrapper types satisfy the expected C++20 concepts.
static_assert(std::input_iterator<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array_range_iterator>);
static_assert(std::input_iterator<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object_range_iterator>);
static_assert(std::ranges::input_range<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array_range>);
static_assert(std::ranges::input_range<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object_range>);
static_assert(std::ranges::view<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array_range>);
static_assert(std::ranges::view<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object_range>);

#endif // SIMDJSON_SUPPORTS_RANGES

#endif // SIMDJSON_GENERIC_ONDEMAND_RANGES_INL_H
