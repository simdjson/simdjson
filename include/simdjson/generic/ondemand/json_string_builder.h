/**
 * This file is part of the builder API. It is temporarily in the ondemand directory
 * but we will move it to a builder directory later.
 */
#ifndef SIMDJSON_GENERIC_STRING_BUILDER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_STRING_BUILDER_H
#include "simdjson/generic/implementation_simdjson_result_base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace builder {

/**
 * A builder for JSON strings representing documents. This is a low-level
 * builder that is not meant to be used directly by end-users. Though it
 * supports atomic types (Booleans, strings), it does not support composed
 * types (arrays and objects).
 *
 * Ultimately, this class should support kernel-specific optimizations. E.g.,
 * it may make use of SIMD instructions to escape strings faster.
 */
class string_builder {
public:
  simdjson_inline string_builder(size_t initial_capacity = 1024);

  /**
   * Append number (includes Booleans). Booleans are mapped to the strings
   * false and true. Numbers are converted to strings abiding by the JSON standard.
   * Floating-point numbers are converted to the shortest string that 'correctly'
   * represents the number.
   */
  template<typename number_type,
         typename = typename std::enable_if<std::is_arithmetic<number_type>::value>::type>
  simdjson_inline void append(number_type v) noexcept;

  /**
   * Append character c.
   */
  simdjson_inline void append(char c)  noexcept;

  /**
   * Append the string 'null'.
   */
  simdjson_inline void append_null()  noexcept;

  /**
   * Clear the content.
   */
  simdjson_inline void clear()  noexcept;

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
  simdjson_inline void append_raw(std::string_view input)  noexcept;

  /**
   * Append len characters from str.
   * There is no UTF-8 validation.
   */
  simdjson_inline void append_raw(const char *str, size_t len) noexcept;
#if SIMDJSON_EXCEPTIONS
  /**
   * Creates an std::string from the written JSON buffer.
   * Throws if memory allocation failed
   *
   * The result may not be valid UTF-8 if some of your content was not valid UTF-8.
   * Use validate_unicode() to check the content if needed.
   */
  simdjson_inline operator std::string() const noexcept(false);

  /**
   * Creates an std::string_view from the written JSON buffer.
   * Throws if memory allocation failed.
   *
   * The result may not be valid UTF-8 if some of your content was not valid UTF-8.
   * Use validate_unicode() to check the content if needed.
   */
  simdjson_inline operator std::string_view() const noexcept(false);
#endif

  /**
   * Returns a view on the written JSON buffer. Returns an error
   * if memory allocation failed.
   *
   * The result may not be valid UTF-8 if some of your content was not valid UTF-8.
   * Use validate_unicode() to check the content.
   */
  simdjson_inline simdjson_result<std::string_view> view() const noexcept;

  /**
   * Appends the null character to the buffer and returns
   * a pointer to the beginning of the written JSON buffer.
   * Returns an error if memory allocation failed.
   * The result is null-terminated.
   *
   * The result may not be valid UTF-8 if some of your content was not valid UTF-8.
   * Use validate_unicode() to check the content.
   */
  simdjson_inline simdjson_result<const char *> c_str() noexcept;

  /**
   * Return true if the content is valid UTF-8.
   */
  simdjson_inline bool validate_unicode() const noexcept;

  /**
   * Returns the current size of the written JSON buffer.
   * If an error occurred, returns 0.
   */
  simdjson_inline size_t size() const noexcept;

private:
  /**
   * Returns true if we can write at least upcoming_bytes bytes.
   * The underlying buffer is reallocated if needed. It is designed
   * to be called before writing to the buffer. It should be fast.
   */
  simdjson_inline bool capacity_check(size_t upcoming_bytes);

  /**
   * Grow the buffer to at least desired_capacity bytes.
   * If the allocation fails, is_valid is set to false. We expect
   * that this function would not be repeatedly called.
   */
  simdjson_inline void grow_buffer(size_t desired_capacity);

  /**
   * We use this helper function to make sure that is_valid is kept consistent.
   */
  simdjson_inline void set_valid(bool valid) noexcept;


  std::unique_ptr<char[]> buffer{};
  size_t position{0};
  size_t capacity{0};
  bool is_valid{true};
};



}
}
} // namespace simdjson

#endif // SIMDJSON_GENERIC_STRING_BUILDER_H