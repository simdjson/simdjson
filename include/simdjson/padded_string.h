#ifndef SIMDJSON_PADDED_STRING_H
#define SIMDJSON_PADDED_STRING_H

#include "simdjson/portability.h"
#include "simdjson/common_defs.h" // for SIMDJSON_PADDING
#include "simdjson/error.h"

#include <cstring>
#include <memory>
#include <string>
#include <ostream>

namespace simdjson {

/**
 * String with extra allocation for ease of use with parser::parse()
 *
 * This is a move-only class, it cannot be copied.
 */
struct padded_string final {

  /**
   * Create a new, empty padded string.
   */
  explicit inline padded_string() noexcept;
  /**
   * Create a new padded string buffer.
   *
   * @param length the size of the string.
   */
  explicit inline padded_string(size_t length) noexcept;
  /**
   * Create a new padded string by copying the given input.
   *
   * @param data the buffer to copy
   * @param length the number of bytes to copy
   */
  explicit inline padded_string(const char *data, size_t length) noexcept;
  /**
   * Create a new padded string by copying the given input.
   *
   * @param str_ the string to copy
   */
  inline padded_string(const std::string & str_ ) noexcept;
  /**
   * Create a new padded string by copying the given input.
   *
   * @param sv_ the string to copy
   */
  inline padded_string(std::string_view sv_) noexcept;
  /**
   * Move one padded string into another.
   *
   * The original padded string will be reduced to zero capacity.
   *
   * @param o the string to move.
   */
  inline padded_string(padded_string &&o) noexcept;
  /**
   * Move one padded string into another.
   *
   * The original padded string will be reduced to zero capacity.
   *
   * @param o the string to move.
   */
  inline padded_string &operator=(padded_string &&o) noexcept;
  inline void swap(padded_string &o) noexcept;
  ~padded_string() noexcept;

  /**
   * The length of the string.
   *
   * Does not include padding.
   */
  size_t size() const noexcept;

  /**
   * The length of the string.
   *
   * Does not include padding.
   */
  size_t length() const noexcept;

  /**
   * The string data.
   **/
  const char *data() const noexcept;

  /**
   * The string data.
   **/
  char *data() noexcept;

  /**
   * Create a std::string_view with the same content.
   */
  operator std::string_view() const;

  /**
   * Load this padded string from a file.
   *
   * @param path the path to the file.
   **/
  inline static simdjson_result<padded_string> load(const std::string &path) noexcept;

private:
  padded_string &operator=(const padded_string &o) = delete;
  padded_string(const padded_string &o) = delete;

  size_t viable_size{0};
  char *data_ptr{nullptr};

}; // padded_string

/**
 * Send padded_string instance to an output stream.
 *
 * @param out The output stream.
 * @param s The padded_string instance.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const padded_string& s) { return out << s.data(); }

#if SIMDJSON_EXCEPTIONS
/**
 * Send padded_string instance to an output stream.
 *
 * @param out The output stream.
 * @param s The padded_string instance.
  * @throw simdjson_error if the result being printed has an error. If there is an error with the
 *        underlying output stream, that error will be propagated (simdjson_error will not be
 *        thrown).
 */
inline std::ostream& operator<<(std::ostream& out, simdjson_result<padded_string> &s) noexcept(false) { return out << s.value(); }
#endif

} // namespace simdjson

// This is deliberately outside of simdjson so that people get it without having to use the namespace
inline simdjson::padded_string operator "" _padded(const char *str, size_t len) {
  return simdjson::padded_string(str, len);
}

namespace simdjson {
namespace internal {

// The allocate_padded_buffer function is a low-level function to allocate memory 
// with padding so we can read past the "length" bytes safely. It is used by 
// the padded_string class automatically. It returns nullptr in case
// of error: the caller should check for a null pointer.
// The length parameter is the maximum size in bytes of the string. 
// The caller is responsible to free the memory (e.g., delete[] (...)).
inline char *allocate_padded_buffer(size_t length) noexcept;

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_PADDED_STRING_H
