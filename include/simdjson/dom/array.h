#ifndef SIMDJSON_DOM_ARRAY_H
#define SIMDJSON_DOM_ARRAY_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/tape_ref.h"
#include "simdjson/minify.h"
#include <ostream>

namespace simdjson {
namespace dom {

class document;
class element;

/**
 * JSON array.
 */
class array {
public:
  /** Create a new, invalid array */
  simdjson_really_inline array() noexcept;

  class iterator {
  public:
    using value_type = element;
    using difference_type = std::ptrdiff_t;

    /**
     * Get the actual value
     */
    inline value_type operator*() const noexcept;
    /**
     * Get the next value.
     *
     * Part of the std::iterator interface.
     *
     */
    inline iterator& operator++() noexcept;
    /**
     * Get the next value.
     *
     * Part of the  std::iterator interface.
     */
    inline iterator operator++(int) noexcept;
    /**
     * Check if these values come from the same place in the JSON.
     *
     * Part of the std::iterator interface.
     */
    inline bool operator!=(const iterator& other) const noexcept;
    inline bool operator==(const iterator& other) const noexcept;

    inline bool operator<(const iterator& other) const noexcept;
    inline bool operator<=(const iterator& other) const noexcept;
    inline bool operator>=(const iterator& other) const noexcept;
    inline bool operator>(const iterator& other) const noexcept;

    iterator() noexcept = default;
    iterator(const iterator&) noexcept = default;
    iterator& operator=(const iterator&) noexcept = default;
  private:
    simdjson_really_inline iterator(const internal::tape_ref &tape) noexcept;
    internal::tape_ref tape;
    friend class array;
  };

  /**
   * Return the first array element.
   *
   * Part of the std::iterable interface.
   */
  inline iterator begin() const noexcept;
  /**
   * One past the last array element.
   *
   * Part of the std::iterable interface.
   */
  inline iterator end() const noexcept;
  /**
   * Get the size of the array (number of immediate children).
   * It is a saturated value with a maximum of 0xFFFFFF: if the value
   * is 0xFFFFFF then the size is 0xFFFFFF or greater.
   */
  inline size_t size() const noexcept;
  /**
   * Get the value associated with the given JSON pointer.  We use the RFC 6901
   * https://tools.ietf.org/html/rfc6901 standard, interpreting the current node
   * as the root of its own JSON document.
   *
   *   dom::parser parser;
   *   array a = parser.parse(R"([ { "foo": { "a": [ 10, 20, 30 ] }} ])"_padded);
   *   a.at_pointer("/0/foo/a/1") == 20
   *   a.at_pointer("0")["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<element> at_pointer(std::string_view json_pointer) const noexcept;

  /**
   * Get the value at the given index. This function has linear-time complexity and
   * is equivalent to the following:
   * 
   *    size_t i=0;
   *    for (auto element : *this) {
   *      if (i == index) { return element; }
   *      i++;
   *    }
   *    return INDEX_OUT_OF_BOUNDS;
   *
   * Avoid calling the at() function repeatedly.
   * 
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  inline simdjson_result<element> at(size_t index) const noexcept;

private:
  simdjson_really_inline array(const internal::tape_ref &tape) noexcept;
  internal::tape_ref tape;
  friend class element;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::minifier;
};

/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const array &value);

} // namespace dom

/** The result of a JSON conversion that may fail. */
template<>
struct simdjson_result<dom::array> : public internal::simdjson_result_base<dom::array> {
public:
  simdjson_really_inline simdjson_result() noexcept; ///< @private
  simdjson_really_inline simdjson_result(dom::array value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  inline simdjson_result<dom::element> at_pointer(std::string_view json_pointer) const noexcept;
  inline simdjson_result<dom::element> at(size_t index) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline dom::array::iterator begin() const noexcept(false);
  inline dom::array::iterator end() const noexcept(false);
  inline size_t size() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

#if SIMDJSON_EXCEPTIONS
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw simdjson_error if the result being printed has an error. If there is an error with the
 *        underlying output stream, that error will be propagated (simdjson_error will not be
 *        thrown).
 */
inline std::ostream& operator<<(std::ostream& out, const simdjson_result<dom::array> &value) noexcept(false);
#endif

} // namespace simdjson

#if defined(__cpp_lib_ranges)
#include <ranges>

namespace std {
namespace ranges {
template<>
inline constexpr bool enable_view<simdjson::dom::array> = true;
#if SIMDJSON_EXCEPTIONS
template<>
inline constexpr bool enable_view<simdjson::simdjson_result<simdjson::dom::array>> = true;
#endif // SIMDJSON_EXCEPTIONS
} // namespace ranges
} // namespace std
#endif // defined(__cpp_lib_ranges)

#endif // SIMDJSON_DOM_ARRAY_H
