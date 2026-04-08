#ifndef SIMDJSON_PADDED_STRING_VIEW_H
#define SIMDJSON_PADDED_STRING_VIEW_H

#include "simdjson/portability.h"
#include "simdjson/base.h" // for SIMDJSON_PADDING
#include "simdjson/error.h"

#include <cstring>
#include <memory>
#include <string>
#include <ostream>
#if SIMDJSON_CPLUSPLUS17
#include <variant>
#endif

namespace simdjson {

/**
 * User-provided string that promises it has extra padded bytes at the end for use with parser::parse().
 */
class padded_string_view : public std::string_view {
private:
  size_t _capacity{0};

public:
  /** Create an empty padded_string_view. */
  inline padded_string_view() noexcept = default;

  /**
   * Promise the given buffer has at least SIMDJSON_PADDING extra bytes allocated to it.
   *
   * @param s The string.
   * @param len The length of the string (not including padding).
   * @param capacity The allocated length of the string, including padding. If the capacity is less
   *       than the length, the capacity will be set to the length.
   */
  explicit inline padded_string_view(const char* s, size_t len, size_t capacity) noexcept;
  /** overload explicit inline padded_string_view(const char* s, size_t len) noexcept */
  explicit inline padded_string_view(const uint8_t* s, size_t len, size_t capacity) noexcept;
#ifdef __cpp_char8_t
  explicit inline padded_string_view(const char8_t* s, size_t len, size_t capacity) noexcept;
#endif
  /**
   * Promise the given string has at least SIMDJSON_PADDING extra bytes allocated to it.
   *
   * The capacity of the string will be used to determine its padding.
   *
   * @param s The string.
   */
  explicit inline padded_string_view(const std::string &s) noexcept;

  /**
   * Promise the given string_view has at least SIMDJSON_PADDING extra bytes allocated to it.
   *
   * @param s The string.
   * @param capacity The allocated length of the string, including padding. If the capacity is less
   *      than the length, the capacity will be set to the length.
   */
  explicit inline padded_string_view(std::string_view s, size_t capacity) noexcept;

  /** The number of allocated bytes. */
  inline size_t capacity() const noexcept;

  /** check that the view has sufficient padding */
  inline bool has_sufficient_padding() const noexcept;

  /**
   * Remove the UTF-8 Byte Order Mark (BOM) if it exists.
   *
   * @return whether a BOM was found and removed
   */
  inline bool remove_utf8_bom() noexcept;

  /** The amount of padding on the string (capacity() - length()) */
  inline size_t padding() const noexcept;

}; // padded_string_view

/**
 * Get the system's memory page size. By default, we return
 * 4096 bytes, which is the most common page size. On systems
 * where the page size is not a multiple of 4096 bytes, and not
 * a unix-like system, nor Windows, this function may return an
 * incorrect value.
 *
 * @return The page size in bytes.
 */
inline uint32_t get_page_size() noexcept;

#if SIMDJSON_CPLUSPLUS17
/**
 * A padded_input is a wrapper around either a padded_string_view or a padded_string.
 * It will automatically pad a string_view if it does not have sufficient padding
 * up to the end of the memory page. Note that a requirement for this method to
 * make sense is to be on a system with a page size of at least 4096 (which is
 * universal except on some embedded systems).
 */
struct padded_input {
    /**
     * Construct a padded_input from a string_view. If the string_view does not have sufficient padding,
     * the data will be copied into a padded_string and the padded_string_view will point to the
     * padded_string's data. Otherwise, the padded_string_view will point to the original string_view's data.
     */
     inline explicit padded_input(std::string_view sv);
    /**
     * Construct a padded_input from a C-style string (length specified). If the string does not have sufficient padding,
     * the data will be copied into a padded_string and the padded_string_view will point to the
     * padded_string's data. Otherwise, the padded_string_view will point to the original string's data.
     */
     inline explicit padded_input(const char *data, size_t length);
    /**
     * Construct a padded_input from a std::string. If the string does not have sufficient padding
     * (considering its capacity), the data will be copied into a padded_string and the padded_string_view
     * will point to the padded_string's data. Otherwise, the padded_string_view will point to the
     * original string's data.
     */
     inline explicit padded_input(const std::string &s);

    /**
     * Check if the padded_input is a view.
     *
     * @return true if the padded_input is a view, false otherwise.
     */
     inline bool is_view() const noexcept;

    /**
     * Convert the padded_input to a padded_string_view.
     *
     * @return The padded_string_view.
     */
     inline operator simdjson::padded_string_view() const noexcept;

private:
    std::variant<simdjson::padded_string_view, simdjson::padded_string> storage;
    // whether we cross a page boundary and need to allocate a new padded string.
    static inline bool needs_allocation(const char* buf, size_t len, size_t padding = SIMDJSON_PADDING) noexcept;
};

#endif // SIMDJSON_CPLUSPLUS17

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
inline std::ostream& operator<<(std::ostream& out, simdjson_result<padded_string_view> &s) noexcept(false);
#endif

/**
 * Create a padded_string_view from a string. The string will be padded with up to SIMDJSON_PADDING
 * space characters. The resulting padded_string_view will have a length equal to the original
 * string, except maybe for trailing white space characters.
 *
 * @param s The string.
 * @return The padded string.
 */
inline padded_string_view pad(std::string& s) noexcept;

/**
 * Create a padded_string_view from a string. The capacity of the string will be padded with SIMDJSON_PADDING
 * characters. The resulting padded_string_view will have a length equal to the original
 * string.
 *
 * @param s The string.
 * @return The padded string.
 */
inline padded_string_view pad_with_reserve(std::string& s) noexcept;

} // namespace simdjson

#endif // SIMDJSON_PADDED_STRING_VIEW_H
