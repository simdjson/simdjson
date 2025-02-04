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
  // We will rarely get here.
  grow_buffer((std::max)(capacity * 2, position + upcoming_bytes));
  // If the buffer allocation failed, we set is_valid to false.
  return is_valid;
}

simdjson_inline void string_builder::grow_buffer(size_t desired_capacity) {
  if (!is_valid) { return; }
  std::unique_ptr<char[]> new_buffer(new (std::nothrow) char[desired_capacity]);
  if (new_buffer.get() == nullptr) {
    set_valid(false);
    return;
  }
  std::memcpy(new_buffer.get(), buffer.get(), position);
  buffer.swap(new_buffer);
  capacity = desired_capacity;
}

simdjson_inline void string_builder::set_valid(bool valid) noexcept {
  if(!valid) {
    is_valid = false;
    capacity = 0;
    position = 0;
    buffer.reset();
  } else {
    is_valid = true;
  }
}

simdjson_inline size_t string_builder::size() const noexcept {
  return position;
}


simdjson_inline void string_builder::append(char c) noexcept {
    if(capacity_check(1)) {
      buffer.get()[position++] = c;
    }
}


simdjson_inline void string_builder::append_null() noexcept {
  constexpr char null_literal[] = "null";
  constexpr size_t null_len = sizeof(null_literal) - 1;
  if(capacity_check(null_len)) {
    std::memcpy(buffer.get() + position, null_literal, null_len);
    position += null_len;
  }
}

simdjson_inline void string_builder::clear()  noexcept {
  position = 0;
  // if it was invalid, we should try to repair it
  if(!is_valid) {
    capacity = 0;
    buffer.reset();
    is_valid = true;
  }
}

namespace internal {

// We could specialize further for 32-bit integers.
int int_log2(uint32_t x) { return (63 - leading_zeroes(x | 1)); }

int fast_digit_count_32(uint32_t x) {
  static uint64_t table[] = {
      4294967296,  8589934582,  8589934582,  8589934582,  12884901788,
      12884901788, 12884901788, 17179868184, 17179868184, 17179868184,
      21474826480, 21474826480, 21474826480, 21474826480, 25769703776,
      25769703776, 25769703776, 30063771072, 30063771072, 30063771072,
      34349738368, 34349738368, 34349738368, 34349738368, 38554705664,
      38554705664, 38554705664, 41949672960, 41949672960, 41949672960,
      42949672960, 42949672960};
  return uint32_t((x + table[int_log2(x)]) >> 32);
}

int int_log2(uint64_t x) { return 63 - leading_zeroes(x | 1); }

int digit_count_64(uint64_t x) {
  static uint64_t table[] = {9,
                             99,
                             999,
                             9999,
                             99999,
                             999999,
                             9999999,
                             99999999,
                             999999999,
                             9999999999,
                             99999999999,
                             999999999999,
                             9999999999999,
                             99999999999999,
                             999999999999999ULL,
                             9999999999999999ULL,
                             99999999999999999ULL,
                             999999999999999999ULL,
                             9999999999999999999ULL};
  int y = (19 * int_log2(x) >> 6);
  y += x > table[y];
  return y + 1;
}

template<typename number_type,
         typename = typename std::enable_if<std::is_unsigned<number_type>::value>::type>
simdjson_inline size_t digit_count(number_type v) noexcept {
  static_assert(sizeof(number_type) == 8
   || sizeof(number_type) == 4
   || sizeof(number_type) == 2
   || sizeof(number_type) == 1, "We only support 8-bit, 16-bit, 32-bit and 64-bit numbers");
  if (sizeof(number_type) <= 4) {
    return fast_digit_count_32(v);
  } else {
    return digit_count_64(v);
  }
}

} // internal

template<typename number_type, typename>
simdjson_inline void string_builder::append(number_type v) noexcept {
  static_assert(std::is_same<number_type, bool>::value
    || std::is_integral<number_type>::value || std::is_floating_point<number_type>::value, "Unsupported number type");
  // If C++17 is available, we can 'if constexpr' here.
  if constexpr (std::is_same<number_type, bool>::value) {
    if (v) {
      constexpr char true_literal[] = "true";
      constexpr size_t true_len = sizeof(true_literal) - 1;
      if(capacity_check(true_len)) {
        std::memcpy(buffer.get() + position, true_literal, true_len);
        position += true_len;
      }
    } else {
      constexpr char false_literal[] = "false";
      constexpr size_t false_len = sizeof(false_literal) - 1;
      if(capacity_check(false_len)) {
        std::memcpy(buffer.get() + position, false_literal, false_len);
        position += false_len;
      }
    }
  } else if constexpr (std::is_unsigned<number_type>::value) {
    constexpr size_t max_number_size = 20;
    if(capacity_check(max_number_size)) {
      using unsigned_type = typename std::make_unsigned<number_type>::type;
      unsigned_type pv = static_cast<unsigned_type>(v);
      size_t dc = internal::digit_count(pv);
      char *write_pointer = buffer.get() + position + dc - 1;
      // optimization opportunity: if v is large, we can do better.
      while(pv >= 10) {
        *write_pointer-- = char('0' + (pv % 10));
        pv /= 10;
      }
      *write_pointer = char('0' + pv);
      position += dc;
    }
  } else if constexpr (std::is_integral<number_type>::value) {
    constexpr size_t max_number_size = 20;
    if(capacity_check(max_number_size)) {
      using unsigned_type = typename std::make_unsigned<number_type>::type;
      bool negative = v < 0;
      unsigned_type pv = static_cast<unsigned_type>(negative ? -v : v);
      size_t dc = internal::digit_count(pv);
      if(negative) {
        buffer.get()[position++] = '-';
      }
      char *write_pointer = buffer.get() + position + dc - 1;
      // optimization opportunity: if v is large, we can do better.
      while(pv >= 10) {
        *write_pointer-- = char('0' + (pv % 10));
        pv /= 10;
      }
      *write_pointer = char('0' + pv);
      position += dc;
    }
  } else if constexpr (std::is_floating_point<number_type>::value) {
    constexpr size_t max_number_size = 24;
    if(capacity_check(max_number_size)) {
      // We could specialize for float.
      char *end = simdjson::internal::to_chars(buffer.get() + position, nullptr, double(v));
      position = end - buffer.get();
    }
  }
}

simdjson_inline void string_builder::escape_and_append(std::string_view input)  noexcept {
  // escaping might turn a control character into \x00xx so 6 characters.
  if(capacity_check(6 * input.size())) {
    position += simdjson::write_string_escaped(input, buffer.get() + position);
  }
}

simdjson_inline void string_builder::escape_and_append_with_quotes(std::string_view input)  noexcept {
  // escaping might turn a control character into \x00xx so 6 characters.
  if(capacity_check(2 + 6 * input.size())) {
    buffer.get()[position++] = '"';
    position += simdjson::write_string_escaped(input, buffer.get() + position);
    buffer.get()[position++] = '"';
  }
}

simdjson_inline void string_builder::append_raw(const char *c)  noexcept {
  size_t len = std::strlen(c);
  append_raw(c, len);
}

simdjson_inline void string_builder::append_raw(std::string_view input)  noexcept {
  if(capacity_check(input.size())) {
    std::memcpy(buffer.get() + position, input.data(), input.size());
    position += input.size();
  }
}

simdjson_inline void string_builder::append_raw(const char *str, size_t len)  noexcept {
  if(capacity_check(len)) {
    std::memcpy(buffer.get() + position, str, len);
    position += len;
  }
}

#if SIMDJSON_EXCEPTIONS
simdjson_inline string_builder::operator std::string() const noexcept(false) {
  return std::string(std::string_view());
}

simdjson_inline string_builder::operator std::string_view() const noexcept(false) {
  return view();
}
#endif

simdjson_inline simdjson_result<std::string_view> string_builder::view() const noexcept {
  if (!is_valid) { return simdjson::OUT_OF_CAPACITY; }
  return std::string_view(buffer.get(), position);
}

simdjson_inline simdjson_result<const char *> string_builder::c_str() noexcept {
  if(capacity_check(1)) {
    buffer.get()[position] = '\0';
    return buffer.get();
  }
  return simdjson::OUT_OF_CAPACITY;
}

simdjson_inline bool string_builder::validate_unicode() const noexcept {
  return simdjson::validate_utf8(buffer.get(), position);
}

} // namespace builder
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_BUILDER_INL_H