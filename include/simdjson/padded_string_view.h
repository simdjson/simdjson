#ifndef SIMDJSON_PADDED_STRING_VIEW_H
#define SIMDJSON_PADDED_STRING_VIEW_H

#include "simdjson/portability.h"
#include "simdjson/common_defs.h" // for SIMDJSON_PADDING
#include "simdjson/error.h"

#include <cstring>
#include <memory>
#include <string>
#include <ostream>

namespace simdjson {

/**
 * User-provided string that promises it has extra padded bytes at the end for use with parser::parse().
 */
struct padded_string_view : public std::string_view {
  /** Create an empty padded_string_view. */
  inline padded_string_view() noexcept = default;

  /**
   * Promise the given buffer has at least SIMDJSON_PADDING extra bytes allocated to it.
   *
   * @param s The string.
   * @param s_len The length of the string (not including padding).
   */
  explicit inline padded_string_view(const char* s, size_t s_len) noexcept;
  /** overload explicit inline padded_string_view(const char* s, size_t s_len) noexcept */
  explicit inline padded_string_view(const uint8_t* s, size_t s_len) noexcept;

  /**
   * Promise the given string_view has at least SIMDJSON_PADDING extra bytes allocated to it.
   *
   * @param s The string.
   */
  explicit inline padded_string_view(std::string_view s) noexcept;
}; // padded_string_view

/**
 * Promise the given buffer has at least SIMDJSON_PADDING extra bytes allocated to it.
 *
 * @param s The string.
 * @param s_len The length of the string (not including padding).
 */
inline padded_string_view promise_padded(const char* s, uint8_t s_len) noexcept;

/**
 * Promise the given string_view has at least SIMDJSON_PADDING extra bytes allocated to it.
 *
 * @param s The string.
 */
inline padded_string_view promise_padded(std::string_view s) noexcept;

#if SIMDJSON_EXCEPTIONS
/**
 * Send padded_string instance to an output stream.
 *
 * @param out The output stream.
 * @param s The padded_string_view.
 * @throw simdjson_error if the result being printed has an error. If there is an error with the
 *        underlying output stream, that error will be propagated (simdjson_error will not be
 *        thrown).
 */
inline std::ostream& operator<<(std::ostream& out, simdjson_result<padded_string_view> &s) noexcept(false) { return out << s.value(); }
#endif

} // namespace simdjson

#endif // SIMDJSON_PADDED_STRING_VIEW_H
