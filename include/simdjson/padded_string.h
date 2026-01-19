#ifndef SIMDJSON_PADDED_STRING_H
#define SIMDJSON_PADDED_STRING_H

#include "simdjson/base.h"
#include "simdjson/error.h"

#include "simdjson/error-inl.h"

#include <cstring>
#include <memory>
#include <string>
#include <ostream>

namespace simdjson {

class padded_string_view;

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
#ifdef __cpp_char8_t
  explicit inline padded_string(const char8_t *data, size_t length) noexcept;
#endif
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
  const uint8_t *u8data() const noexcept { return static_cast<const uint8_t*>(static_cast<const void*>(data_ptr));}

  /**
   * The string data.
   **/
  char *data() noexcept;

  /**
   * Append data to the padded string. Return true on success, false on failure.
   * The complexity is O(n) where n is the new size of the string. If you are
   * doing multiple appends, consider using padded_string_builder for better performance.
   *
   * @param data the buffer to append
   * @param length the number of bytes to append
   */
  inline bool append(const char *data, size_t length) noexcept;

  /**
   * Create a std::string_view with the same content.
   */
  operator std::string_view() const;

  /**
   * Create a padded_string_view with the same content.
   */
  operator padded_string_view() const noexcept;

  /**
   * Load this padded string from a file.
   *
   * ## Windows and Unicode
   *
   * Windows users who need to read files with non-ANSI characters in the
   * name should set their code page to UTF-8 (65001) before calling this
   * function. This should be the default with Windows 11 and better.
   * Further, they may use the AreFileApisANSI function to determine whether
   * the filename is interpreted using the ANSI or the system default OEM
   * codepage, and they may call SetFileApisToOEM accordingly.
   *
   * @return IO_ERROR on error. Be mindful that on some 32-bit systems,
   * the file size might be limited to 2 GB.
   *
   * @param path the path to the file.
   **/
  inline static simdjson_result<padded_string> load(std::string_view path) noexcept;

    #if defined(_WIN32) && SIMDJSON_CPLUSPLUS17
  /**
   * This function accepts a wide string path (UTF-16) and converts it to
   * UTF-8 before loading the file. This allows windows users to work
   * with unicode file paths without manually converting the paths every time.
   *
   * @return IO_ERROR on error, including conversion failures.
   *
   * @param path the path to the file as a wide string.
  **/
    inline static simdjson_result<padded_string> load(std::wstring_view path) noexcept;
  #endif

private:
  friend class padded_string_builder;
  padded_string &operator=(const padded_string &o) = delete;
  padded_string(const padded_string &o) = delete;

  size_t viable_size{0};
  char *data_ptr{nullptr};

}; // padded_string

/**
 * Builder for constructing padded_string incrementally.
 *
 * This class allows efficient appending of data and then building a padded_string.
 */
class padded_string_builder {
public:
  /**
   * Create a new, empty padded string builder.
   */
  inline padded_string_builder() noexcept;

  /**
   * Create a new padded string builder with initial capacity.
   *
   * @param capacity the initial capacity of the builder.
   */
  inline padded_string_builder(size_t capacity) noexcept;

  /**
   * Move constructor.
   */
  inline padded_string_builder(padded_string_builder &&o) noexcept;

  /**
   * Move assignment.
   */
  inline padded_string_builder &operator=(padded_string_builder &&o) noexcept;

  /**
   * Copy constructor (deleted).
   */
  padded_string_builder(const padded_string_builder &) = delete;

  /**
   * Copy assignment (deleted).
   */
  padded_string_builder &operator=(const padded_string_builder &) = delete;

  /**
   * Destructor.
   */
  inline ~padded_string_builder() noexcept;

  /**
   * Append data to the builder.
   *
   * @param newdata the buffer to append
   * @param length the number of bytes to append
   * @return true if the append succeeded, false if allocation failed
   */
  inline bool append(const char *newdata, size_t length) noexcept;

  /**
   * Append a string view to the builder.
   *
   * @param sv the string view to append
   * @return true if the append succeeded, false if allocation failed
   */
  inline bool append(std::string_view sv) noexcept;

  /**
   * Get the current length of the built string.
   */
  inline size_t length() const noexcept;

  /**
   * Build a padded_string from the current content. The builder's content
   * is not modified. If you want to avoid the copy, use convert() instead.
   *
   * @return a padded_string containing a copy of the built content.
   */
  inline padded_string build() const noexcept;

  /**
   * Convert the current content into a padded_string. The
   * builder's content is emptied, the capacity is lost.
   *
   * @return a padded_string containing the built content.
   */
  inline padded_string convert() noexcept;
private:
  size_t size{0};
  size_t capacity{0};
  char *data{nullptr};

  /**
   * Ensure the builder has enough capacity.
   *
   * @param additional the additional capacity needed.
   * @return true if the reservation succeeded, false if allocation failed
   */
  inline bool reserve(size_t additional) noexcept;
};

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
inline simdjson::padded_string operator ""_padded(const char *str, size_t len);
#ifdef __cpp_char8_t
inline simdjson::padded_string operator ""_padded(const char8_t *str, size_t len);
#endif

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
