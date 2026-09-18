#ifndef SIMDJSON_DOM_ARRAY_H
#define SIMDJSON_DOM_ARRAY_H

#include <vector>

#include "simdjson/dom/base.h"
#include "simdjson/internal/tape_ref.h"

namespace simdjson {
namespace dom {

/**
 * JSON array.
 */
class array {
public:
  /** Create a new, invalid array */
  simdjson_inline array() noexcept;

  class iterator {
  public:
    using value_type = element;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;
    using iterator_category = std::forward_iterator_tag;

    /**
     * Get the actual value
     */
    inline reference operator*() const noexcept;
    /**
     * Get the next value.
     *
     * Part of the std::iterator interface.
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
    simdjson_inline iterator(const internal::tape_ref &tape) noexcept;
    internal::tape_ref tape{};
    friend class array;
  };

  /**
   * A forward iterator that visits the array's elements in reverse order.
   * Like iterator, it returns element handles by value and does not own the
   * document. No allocation or modification of the document is performed.
   */
  class reverse_iterator {
  public:
    using value_type = element;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;
    using iterator_category = std::forward_iterator_tag;

    inline reference operator*() const noexcept;
    inline reverse_iterator& operator++() noexcept;
    inline reverse_iterator operator++(int) noexcept;
    inline bool operator==(const reverse_iterator& other) const noexcept;
    inline bool operator!=(const reverse_iterator& other) const noexcept;

    reverse_iterator() noexcept = default;
    reverse_iterator(const reverse_iterator&) noexcept = default;
    reverse_iterator& operator=(const reverse_iterator&) noexcept = default;
  private:
    simdjson_inline reverse_iterator(const internal::tape_ref &tape, size_t array_start) noexcept;
    internal::tape_ref tape{};
    size_t array_start{};
    friend class array;
  };

  /**
   * Return the last array element, or rend() for an empty array.
   * Incrementing the returned iterator moves toward the first element.
   * A complete traversal takes O(n) time and O(1) additional space, where n
   * is the number of immediate elements in the array.
   * Finding the last or previous element can take O(n) time in the worst case when
   * numeric payloads equal numeric type markers. Increments are amortized O(1)
   * over a complete traversal; nested values do not increase this bound.
   */
  inline reverse_iterator rbegin() const noexcept;
  /** Return the reverse traversal sentinel, before the first element. */
  inline reverse_iterator rend() const noexcept;

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
   * Get the total number of slots used by this array on the tape.
   *
   * Note that this is not the same thing as `size()`, which reports the
   * number of actual elements within an array (not counting its children).
   *
   * Since an element can use 1 or 2 slots on the tape, you can only use this
   * to figure out the total size of an array (including its children,
   * recursively) if you know its structure ahead of time.
   **/
  inline size_t number_of_slots() const noexcept;
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
   * Recursive function which processes the JSON path of each child element
   */
  inline void process_json_path_of_child_elements(std::vector<element>::iterator& current, std::vector<element>::iterator& end, const std::string_view& path_suffix, std::vector<element>& accumulator) const noexcept;


  /**
   * Adds support for JSONPath expression with wildcards '*'
   */
  inline simdjson_result<std::vector<element>> at_path_with_wildcard(std::string_view json_path) const noexcept;

  /**
   * Get the value associated with the given JSONPath expression. We only support
   * JSONPath queries that trivially convertible to JSON Pointer queries: key
   * names and array indices.
   *
   * https://www.rfc-editor.org/rfc/rfc9535 (RFC 9535)
   *
   * @return The value associated with the given JSONPath expression, or:
   *         - INVALID_JSON_POINTER if the JSONPath to JSON Pointer conversion fails
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
  */
  inline simdjson_result<element> at_path(std::string_view json_path) const noexcept;

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

  /**
   * Gets the values of items in an array element
   * This function has linear-time complexity: the values are checked one by one.
   *
   * @return The child elements of an array
   */

  inline std::vector<element>& get_values(std::vector<element>& out) const noexcept;

  /**
   * Implicitly convert object to element
   */
  inline operator element() const noexcept;

private:
  simdjson_inline array(const internal::tape_ref &tape) noexcept;
  internal::tape_ref tape{};
  friend class element;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::internal::string_builder;
};


} // namespace dom

/** The result of a JSON conversion that may fail. */
template<>
struct simdjson_result<dom::array> : public internal::simdjson_result_base<dom::array> {
public:
  simdjson_inline simdjson_result() noexcept; ///< @private
  simdjson_inline simdjson_result(dom::array value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private

  inline simdjson_result<dom::element> at_pointer(std::string_view json_pointer) const noexcept;
  inline void process_json_path_of_child_elements(std::vector<dom::element>::iterator& current, std::vector<dom::element>::iterator& end, const std::string_view& path_suffix, std::vector<dom::element>& accumulator) const noexcept;
  inline simdjson_result<std::vector<dom::element>> at_path_with_wildcard(std::string_view json_path) const noexcept;
  inline simdjson_result<dom::element> at_path(std::string_view json_path) const noexcept;
  inline simdjson_result<dom::element> at(size_t index) const noexcept;
  inline std::vector<dom::element>& get_values(std::vector<dom::element>& out) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline dom::array::iterator begin() const noexcept(false);
  inline dom::array::iterator end() const noexcept(false);
  inline dom::array::reverse_iterator rbegin() const noexcept(false);
  inline dom::array::reverse_iterator rend() const noexcept(false);
  inline size_t size() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};



} // namespace simdjson

#if SIMDJSON_SUPPORTS_RANGES
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
#endif // SIMDJSON_SUPPORTS_RANGES

#endif // SIMDJSON_DOM_ARRAY_H
