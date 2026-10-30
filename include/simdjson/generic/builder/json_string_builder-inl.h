#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <type_traits>
#ifndef SIMDJSON_GENERIC_STRING_BUILDER_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_STRING_BUILDER_INL_H
#include "simdjson/generic/builder/json_string_builder.h"
#include "simdjson/internal/jeaiii.h"
#include "simdjson/internal/json_escape.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace builder {

using simdjson::internal::write_string_escaped;

namespace internal {
// Sets the size of s to n without initializing the new bytes when the
// standard library allows it (C++23 resize_and_overwrite).
simdjson_inline void resize_uninitialized(std::string &s, size_t n) {
#if defined(__cpp_lib_string_resize_and_overwrite) && __cpp_lib_string_resize_and_overwrite >= 202110L
  s.resize_and_overwrite(n, [](char *, size_t len) noexcept { return len; });
#else
  s.resize(n);
#endif
}
} // namespace internal

simdjson_inline string_builder::string_builder(size_t initial_capacity) {
  grow_buffer(initial_capacity);
}

simdjson_inline bool string_builder::capacity_check(size_t upcoming_bytes) {
  // We use the convention that when is_valid is false, then the capacity and
  // the position are 0.
  // Most of the time, this function will return true.
  if (simdjson_likely(upcoming_bytes <= capacity - position)) {
    return true;
  }
  // check for overflow, most of the time there is no overflow
  if (simdjson_unlikely(position + upcoming_bytes < position)) {
    return false;
  }
  // We will rarely get here.
  grow_buffer((std::max)(capacity * 2, position + upcoming_bytes));
  // If the buffer allocation failed, we set is_valid to false.
  return is_valid;
}

inline void string_builder::grow_buffer(size_t desired_capacity) {
  if (!is_valid) {
    return;
  }
  std::string new_buffer;
  if (desired_capacity > new_buffer.max_size()) {
    set_valid(false);
    return;
  }
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {
    internal::resize_uninitialized(new_buffer, desired_capacity);
  } catch (...) { // std::bad_alloc
    set_valid(false);
    return;
  }
#else
  // Without exceptions, a failed allocation terminates the program.
  internal::resize_uninitialized(new_buffer, desired_capacity);
#endif
  std::memcpy(&new_buffer[0], buffer_data(), position);
  buffer.swap(new_buffer);
  capacity = desired_capacity;
}

// Not simdjson_inline: MSVC cannot force-inline a function with a try block (C4714).
inline error_code string_builder::move_to(std::string &out) noexcept {
  if (!is_valid) {
    return simdjson::OUT_OF_CAPACITY;
  }
  if (position <= buffer.capacity() / 2) {
    // Most of the buffer is unused: copy the bytes out, so that a small
    // result does not keep the whole buffer allocated. The builder keeps its
    // buffer for what it writes next.
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try {
      out.assign(buffer.data(), position);
    } catch (...) { // std::bad_alloc
      return simdjson::MEMALLOC;
    }
#else
    // Without exceptions, a failed allocation terminates the program.
    out.assign(buffer.data(), position);
#endif
    position = 0;
    return simdjson::SUCCESS;
  }
  buffer.resize(position); // shrinking never allocates
  out = std::move(buffer);
  std::string().swap(buffer);
  position = 0;
  capacity = 0;
  return simdjson::SUCCESS;
}

simdjson_inline void string_builder::set_valid(bool valid) noexcept {
  if (!valid) {
    is_valid = false;
    capacity = 0;
    position = 0;
    std::string().swap(buffer);
  } else {
    is_valid = true;
  }
}

simdjson_inline size_t string_builder::size() const noexcept {
  return position;
}

simdjson_inline void string_builder::append(char c) noexcept {
  if (capacity_check(1)) {
    buffer_data()[position++] = c;
  }
}

simdjson_inline void string_builder::append_null() noexcept {
  constexpr char null_literal[] = "null";
  constexpr size_t null_len = sizeof(null_literal) - 1;
  if (capacity_check(null_len)) {
    std::memcpy(buffer_data() + position, null_literal, null_len);
    position += null_len;
  }
}

simdjson_inline void string_builder::clear() noexcept {
  position = 0;
  // if it was invalid, we should try to repair it
  if (!is_valid) {
    capacity = 0;
    std::string().swap(buffer);
    is_valid = true;
  }
}

namespace internal {

using simdjson::internal::write_uint_jeaiii;

// Writes v at p, which must have to_chars_buffer_size bytes available, and
// returns the end of what was written.
simdjson_inline char *write_double(char *p, double v) noexcept {
#if SIMDJSON_ENABLE_NAN_INF
  if (simdjson_unlikely(!std::isfinite(v))) {
    if (std::isnan(v)) {
      std::memcpy(p, "NaN", 3);
      return p + 3;
    }
    if (v < 0) {
      *p++ = '-';
    }
    std::memcpy(p, "Infinity", 8);
    return p + 8;
  }
#endif
  return simdjson::internal::to_chars(p, nullptr, v);
}
} // namespace internal

template <typename number_type, typename>
simdjson_inline void string_builder::append(number_type v) noexcept {
  static_assert(std::is_same<number_type, bool>::value ||
                    std::is_integral<number_type>::value ||
                    std::is_floating_point<number_type>::value,
                "Unsupported number type");
  static_assert(sizeof(number_type) <= sizeof(uint64_t),
                "Integers wider than 64 bits are not supported");
  // If C++17 is available, we can 'if constexpr' here.
  SIMDJSON_IF_CONSTEXPR(std::is_same<number_type, bool>::value) {
    if (v) {
      constexpr char true_literal[] = "true";
      constexpr size_t true_len = sizeof(true_literal) - 1;
      if (capacity_check(true_len)) {
        std::memcpy(buffer_data() + position, true_literal, true_len);
        position += true_len;
      }
    } else {
      constexpr char false_literal[] = "false";
      constexpr size_t false_len = sizeof(false_literal) - 1;
      if (capacity_check(false_len)) {
        std::memcpy(buffer_data() + position, false_literal, false_len);
        position += false_len;
      }
    }
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_unsigned<number_type>::value) {
    constexpr size_t max_number_size = 20;
    if (capacity_check(max_number_size)) {
      using unsigned_type = typename std::make_unsigned<number_type>::type;
      char* end = internal::write_uint_jeaiii(
          buffer_data() + position,
          static_cast<uint64_t>(static_cast<unsigned_type>(v)));
      position = end - buffer_data();
    }
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_integral<number_type>::value) {
    // 19 digits (max abs value of int64_t) + optional minus sign.
    constexpr size_t max_number_size = 20;
    if (capacity_check(max_number_size)) {
      using unsigned_type = typename std::make_unsigned<number_type>::type;
      bool negative = v < 0;
      // 0 - pv (rather than -pv) avoids an MSVC unary-minus warning.
      unsigned_type pv = negative
          ? unsigned_type(0) - static_cast<unsigned_type>(v)
          : static_cast<unsigned_type>(v);
      // Branchless: always write '-', advance only if negative.
      buffer_data()[position] = '-';
      position += negative;
      char* end = internal::write_uint_jeaiii(
          buffer_data() + position, static_cast<uint64_t>(pv));
      position = end - buffer_data();
    }
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_floating_point<number_type>::value) {
    // Must reserve to_chars_buffer_size (40): only ~24 chars are emitted,
    // but to_chars over-writes with fixed-size 16/17-byte copies so the
    // compiler can inline mem* (see simdjson::internal::to_chars_buffer_size).
    constexpr size_t max_number_size = simdjson::internal::to_chars_buffer_size;
    if (capacity_check(max_number_size)) {
#if SIMDJSON_ENABLE_NAN_INF
      // Check if the input might be NaN or infinity
      if (simdjson_unlikely(!std::isfinite(v))) {
        if (std::isnan(v)) {
          constexpr char nan_literal[] = "NaN";
          constexpr size_t nan_len = sizeof(nan_literal) - 1;

          std::memcpy(buffer_data() + position, nan_literal, nan_len);
          position += nan_len;
        } else {
          constexpr char inf_literal[] = "Infinity";
          constexpr size_t inf_len = sizeof(inf_literal) - 1;
          if (v < 0) {
            buffer_data()[position] = '-';
            ++position;
          }
          std::memcpy(buffer_data() + position, inf_literal, inf_len);
          position += inf_len;
        }
        return;
      }
#endif

      // We could specialize for float.
      char *end = simdjson::internal::to_chars(buffer_data() + position, nullptr,
                                               double(v));
      position = end - buffer_data();
    }
  }
}

simdjson_inline void
string_builder::escape_and_append(std::string_view input) noexcept {
  // escaping might turn a control character into \x00xx so 6 characters.
  // Guard against size_t overflow in the multiplication below.
  if (input.size() > (std::numeric_limits<size_t>::max)() / 6) {
    set_valid(false);
    return;
  }
  if (capacity_check(6 * input.size())) {
    position += write_string_escaped(input, buffer_data() + position);
  }
}

simdjson_inline void
string_builder::escape_and_append_with_quotes(std::string_view input) noexcept {
  // escaping might turn a control character into \x00xx so 6 characters.
  // Guard against size_t overflow in the arithmetic below.
  if (input.size() > ((std::numeric_limits<size_t>::max)() - 2) / 6) {
    set_valid(false);
    return;
  }
  if (capacity_check(2 + 6 * input.size())) {
    buffer_data()[position++] = '"';
    position += write_string_escaped(input, buffer_data() + position);
    buffer_data()[position++] = '"';
  }
}

simdjson_inline void
string_builder::escape_and_append_with_quotes(char input) noexcept {
  // escaping might turn a control character into \x00xx so 6 characters.
  if (capacity_check(2 + 6 * 1)) {
    buffer_data()[position++] = '"';
    std::string_view cinput(&input, 1);
    position += write_string_escaped(cinput, buffer_data() + position);
    buffer_data()[position++] = '"';
  }
}

simdjson_inline void
string_builder::escape_and_append_with_quotes(const char *input) noexcept {
  std::string_view cinput(input);
  escape_and_append_with_quotes(cinput);
}
#if SIMDJSON_SUPPORTS_CONCEPTS
template <constevalutil::fixed_string key>
simdjson_inline void string_builder::escape_and_append_with_quotes() noexcept {
#if SIMDJSON_CONSTEVAL
  append_static_string<constevalutil::quoted_escaped_text<key>>();
#else
  escape_and_append_with_quotes(constevalutil::string_constant<key>::value);
#endif
}
#endif

simdjson_inline void string_builder::append_raw(const char *c) noexcept {
  // char_traits::length is constexpr; lets the compiler fold the length
  // when called with a pointer to a compile-time-constant string.
  size_t len = std::char_traits<char>::length(c);
  append_raw(c, len);
}

simdjson_inline void
string_builder::append_raw(std::string_view input) noexcept {
  if (capacity_check(input.size())) {
    std::memcpy(buffer_data() + position, input.data(), input.size());
    position += input.size();
  }
}

simdjson_inline void string_builder::append_raw(const char *str,
                                                size_t len) noexcept {
  if (capacity_check(len)) {
    std::memcpy(buffer_data() + position, str, len);
    position += len;
  }
}

template <size_t N>
simdjson_inline void string_builder::append_raw_n(const char *str) noexcept {
  if (capacity_check(N)) {
    std::memcpy(buffer_data() + position, str, N);
    position += N;
  }
}

#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_CONSTEVAL
template <typename Text>
simdjson_inline void string_builder::append_static_string() noexcept {
  constexpr auto& padded = constevalutil::static_string<Text>;
  if (capacity_check(padded.size())) {
    std::memcpy(buffer_data() + position, padded.data(), padded.size());
    position += constevalutil::static_string_size<Text>();
  }
}
#endif

#if SIMDJSON_SUPPORTS_CONCEPTS
// Support for optional types (std::optional, etc.)
template <concepts::optional_type T>
  requires(!require_custom_serialization<T>)
simdjson_inline void string_builder::append(const T &opt) {
  if (opt) {
    append(*opt);
  } else {
    append_null();
  }
}

template <typename T>
  requires(require_custom_serialization<T>)
simdjson_inline void string_builder::append(T &&val) {
  serialize(*this, std::forward<T>(val));
}

template <typename T>
  requires(std::is_convertible<T, std::string_view>::value ||
           std::is_same<T, const char *>::value)
simdjson_inline void string_builder::append(const T &value) {
  escape_and_append_with_quotes(value);
}
#endif

#if SIMDJSON_SUPPORTS_RANGES && SIMDJSON_SUPPORTS_CONCEPTS
// Support for range-based appending (std::ranges::view, etc.)
template <std::ranges::range R>
  requires(!std::is_convertible<R, std::string_view>::value && !concepts::optional_type<R> && !require_custom_serialization<R>)
simdjson_inline void string_builder::append(const R &range) noexcept {
  auto it = std::ranges::begin(range);
  auto end = std::ranges::end(range);
  if constexpr (concepts::is_pair<std::ranges::range_value_t<R>>) {
    start_object();

    if (it == end) {
      end_object();
      return; // Handle empty range
    }
    // Append first item without leading comma
    append_key_value(it->first, it->second);
    ++it;

    // Append remaining items with preceding commas
    for (; it != end; ++it) {
      append_comma();
      append_key_value(it->first, it->second);
    }
    end_object();
  } else {
    start_array();
    if (it == end) {
      end_array();
      return; // Handle empty range
    }

    // Append first item without leading comma
    append(*it);
    ++it;

    // Append remaining items with preceding commas
    for (; it != end; ++it) {
      append_comma();
      append(*it);
    }
    end_array();
  }
}

#endif

#if SIMDJSON_EXCEPTIONS
simdjson_inline string_builder::operator std::string() const noexcept(false) {
  return std::string(operator std::string_view());
}

simdjson_inline string_builder::operator std::string_view() const
    noexcept(false) simdjson_lifetime_bound {
  return view();
}
#endif

simdjson_inline simdjson_result<std::string_view>
string_builder::view() const noexcept {
  if (!is_valid) {
    return simdjson::OUT_OF_CAPACITY;
  }
  return std::string_view(buffer_data(), position);
}

simdjson_inline simdjson_result<const char *> string_builder::c_str() noexcept {
  if (capacity_check(1)) {
    buffer_data()[position] = '\0';
    return buffer_data();
  }
  return simdjson::OUT_OF_CAPACITY;
}

simdjson_inline bool string_builder::validate_unicode() const noexcept {
  return simdjson::validate_utf8(buffer_data(), position);
}

simdjson_inline void string_builder::start_object() noexcept {
  if (capacity_check(1)) {
    buffer_data()[position++] = '{';
  }
}

simdjson_inline void string_builder::end_object() noexcept {
  if (capacity_check(1)) {
    buffer_data()[position++] = '}';
  }
}

simdjson_inline void string_builder::start_array() noexcept {
  if (capacity_check(1)) {
    buffer_data()[position++] = '[';
  }
}

simdjson_inline void string_builder::end_array() noexcept {
  if (capacity_check(1)) {
    buffer_data()[position++] = ']';
  }
}

simdjson_inline void string_builder::append_comma() noexcept {
  if (capacity_check(1)) {
    buffer_data()[position++] = ',';
  }
}

simdjson_inline void string_builder::append_colon() noexcept {
  if (capacity_check(1)) {
    buffer_data()[position++] = ':';
  }
}

template <typename key_type, typename value_type>
simdjson_inline void
string_builder::append_key_value(key_type key, value_type value) noexcept {
  static_assert(std::is_same<key_type, const char *>::value ||
                    std::is_convertible<key_type, std::string_view>::value,
                "Unsupported key type");
  escape_and_append_with_quotes(key);
  append_colon();
  SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, std::nullptr_t>::value) {
    append_null();
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, char>::value) {
    escape_and_append_with_quotes(value);
  }
  else SIMDJSON_IF_CONSTEXPR(
      std::is_convertible<value_type, std::string_view>::value) {
    escape_and_append_with_quotes(value);
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, const char *>::value) {
    escape_and_append_with_quotes(value);
  }
  else {
    append(value);
  }
}

#if SIMDJSON_SUPPORTS_CONCEPTS
#if SIMDJSON_CONSTEVAL
template <constevalutil::fixed_string key>
struct quoted_key_text {
  static consteval std::string text() {
    return constevalutil::consteval_to_quoted_escaped(key.view()) + ":";
  }
};
#endif

template <constevalutil::fixed_string key, typename value_type>
simdjson_inline void
string_builder::append_key_value(value_type value) noexcept {
#if SIMDJSON_CONSTEVAL
  append_static_string<quoted_key_text<key>>();
#else
  escape_and_append_with_quotes<key>();
  append_colon();
#endif
  SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, std::nullptr_t>::value) {
    append_null();
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, char>::value) {
    escape_and_append_with_quotes(value);
  }
  else SIMDJSON_IF_CONSTEXPR(
      std::is_convertible<value_type, std::string_view>::value) {
    escape_and_append_with_quotes(value);
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, const char *>::value) {
    escape_and_append_with_quotes(value);
  }
  else {
    append(value);
  }
}

#if SIMDJSON_CONSTEVAL
template <typename T>
concept compile_time_formattable =
  constevalutil::is_fixed_string<std::remove_cvref_t<T>> ||
  std::same_as<std::remove_cvref_t<T>, bool> ||
  std::same_as<std::remove_cvref_t<T>, std::nullptr_t>;

template <constevalutil::fixed_string key, auto value>
requires compile_time_formattable<decltype(value)>
struct key_value_text {
  static consteval std::string text() {
    using value_type = std::remove_cvref_t<decltype(value)>;
    std::string out = constevalutil::consteval_to_quoted_escaped(key.view()) + ':';
    SIMDJSON_IF_CONSTEXPR(constevalutil::is_fixed_string<value_type>)
      out += constevalutil::consteval_to_quoted_escaped(value.view());
    else SIMDJSON_IF_CONSTEXPR(std::same_as<value_type, bool>)
      out += value ? "true" : "false";
    else SIMDJSON_IF_CONSTEXPR(std::same_as<value_type, std::nullptr_t>)
      out += "null";
    return out;
  }
};
#endif

template <constevalutil::fixed_string key, constevalutil::fixed_string value>
simdjson_inline void
string_builder::append_key_value() noexcept {
#if SIMDJSON_CONSTEVAL
  append_static_string<key_value_text<key, value>>();
#else
  escape_and_append_with_quotes<key>();
  append_colon();
  escape_and_append_with_quotes<value>();
#endif
}

template <constevalutil::fixed_string key, auto value>
requires (!constevalutil::is_fixed_string<std::remove_cvref_t<decltype(value)>>)
simdjson_inline void
string_builder::append_key_value() noexcept {
#if SIMDJSON_CONSTEVAL
  SIMDJSON_IF_CONSTEXPR(compile_time_formattable<decltype(value)>) {
    append_static_string<key_value_text<key, value>>();
  } else
#endif
  {
    append_key_value<key>(value);
  }
}
#endif

} // namespace builder
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_STRING_BUILDER_INL_H
