#ifndef SIMDJSON_DOM_OBJECT_H
#define SIMDJSON_DOM_OBJECT_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/element.h"
#include "simdjson/internal/tape_ref.h"

namespace simdjson {
namespace dom {

/**
 * JSON object.
 */
class object {
public:
  /** Create a new, invalid object */
  simdjson_inline object() noexcept;

  class iterator {
  public:
    using value_type = const key_value_pair;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;
    using iterator_category = std::forward_iterator_tag;

    /**
     * Get the actual key/value pair
     */
    inline reference operator*() const noexcept;
    /**
     * Get the next key/value pair.
     *
     * Part of the std::iterator interface.
     *
     */
    inline iterator& operator++() noexcept;
    /**
     * Get the next key/value pair.
     *
     * Part of the std::iterator interface.
     *
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
    /**
     * Get the key of this key/value pair.
     */
    inline std::string_view key() const noexcept;
    /**
     * Get the length (in bytes) of the key in this key/value pair.
     * You should expect this function to be faster than key().size().
     */
    inline uint32_t key_length() const noexcept;
    /**
     * Returns true if the key in this key/value pair is equal
     * to the provided string_view.
     */
    inline bool key_equals(std::string_view o) const noexcept;
    /**
     * Returns true if the key in this key/value pair is equal
     * to the provided string_view in a case-insensitive manner.
     * Case comparisons may only be handled correctly for ASCII strings.
     */
    inline bool key_equals_case_insensitive(std::string_view o) const noexcept;
    /**
     * Get the key of this key/value pair.
     */
    inline const char *key_c_str() const noexcept;
    /**
     * Get the value of this key/value pair.
     */
    inline element value() const noexcept;

    iterator() noexcept = default;
    iterator(const iterator&) noexcept = default;
    iterator& operator=(const iterator&) noexcept = default;
  private:
    simdjson_inline iterator(const internal::tape_ref &tape) noexcept;

    internal::tape_ref tape;

    friend class object;
  };

  /**
   * Return the first key/value pair.
   *
   * Part of the std::iterable interface.
   */
  inline iterator begin() const noexcept;
  /**
   * One past the last key/value pair.
   *
   * Part of the std::iterable interface.
   */
  inline iterator end() const noexcept;
  /**
   * Get the size of the object (number of keys).
   * It is a saturated value with a maximum of 0xFFFFFF: if the value
   * is 0xFFFFFF then the size is 0xFFFFFF or greater.
   */
  inline size_t size() const noexcept;
  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   int64_t(parser.parse(R"({ "a\n": 1 })"_padded)["a\n"]) == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](std::string_view key) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   int64_t(parser.parse(R"({ "a\n": 1 })"_padded)["a\n"]) == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](const char *key) const noexcept;
  simdjson_result<element> operator[](int) const noexcept = delete;

  /**
   * Get the value associated with the given JSON pointer. We use the RFC 6901
   * https://tools.ietf.org/html/rfc6901 standard, interpreting the current node
   * as the root of its own JSON document.
   *
   *   dom::parser parser;
   *   object obj = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})"_padded);
   *   obj.at_pointer("/foo/a/1") == 20
   *   obj.at_pointer("/foo")["a"].at(1) == 20
   *
   * It is allowed for a key to be the empty string:
   *
   *   dom::parser parser;
   *   object obj = parser.parse(R"({ "": { "a": [ 10, 20, 30 ] }})"_padded);
   *   obj.at_pointer("//a/1") == 20
   *   obj.at_pointer("/")["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<element> at_pointer(std::string_view json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSONPath expression. We only support
   * JSONPath queries that trivially convertible to JSON Pointer queries: key
   * names and array indices.
   *
   * https://datatracker.ietf.org/doc/html/draft-normington-jsonpath-00
   *
   * @return The value associated with the given JSONPath expression, or:
   *         - INVALID_JSON_POINTER if the JSONPath to JSON Pointer conversion fails
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
  */
  inline simdjson_result<element> at_path(std::string_view json_path) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   int64_t(parser.parse(R"({ "a\n": 1 })"_padded)["a\n"]) == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key(std::string_view key) const noexcept;

  /**
   * Get the value associated with the given key in a case-insensitive manner.
   * It is only guaranteed to work over ASCII inputs.
   *
   * Note: The key will be matched against **unescaped** JSON.
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key_case_insensitive(std::string_view key) const noexcept;

  /**
   * Implicitly convert object to element
   */
  inline operator element() const noexcept;

private:
  simdjson_inline object(const internal::tape_ref &tape) noexcept;

  internal::tape_ref tape;

  friend class element;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::internal::string_builder;
};

/**
 * Key/value pair in an object.
 */
class key_value_pair {
public:
  /** key in the key-value pair **/
  std::string_view key;
  /** value in the key-value pair **/
  element value;

private:
  simdjson_inline key_value_pair(std::string_view _key, element _value) noexcept;
  friend class object;
};

} // namespace dom

/** The result of a JSON conversion that may fail. */
template<>
struct simdjson_result<dom::object> : public internal::simdjson_result_base<dom::object> {
public:
  simdjson_inline simdjson_result() noexcept; ///< @private
  simdjson_inline simdjson_result(dom::object value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private

  inline simdjson_result<dom::element> operator[](std::string_view key) const noexcept;
  inline simdjson_result<dom::element> operator[](const char *key) const noexcept;
  simdjson_result<dom::element> operator[](int) const noexcept = delete;
  inline simdjson_result<dom::element> at_pointer(std::string_view json_pointer) const noexcept;
  inline simdjson_result<dom::element> at_path(std::string_view json_path) const noexcept;
  inline simdjson_result<dom::element> at_key(std::string_view key) const noexcept;
  inline simdjson_result<dom::element> at_key_case_insensitive(std::string_view key) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline dom::object::iterator begin() const noexcept(false);
  inline dom::object::iterator end() const noexcept(false);
  inline size_t size() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

} // namespace simdjson

#if defined(__cpp_lib_ranges)
#include <ranges>

namespace std {
namespace ranges {
template<>
inline constexpr bool enable_view<simdjson::dom::object> = true;
#if SIMDJSON_EXCEPTIONS
template<>
inline constexpr bool enable_view<simdjson::simdjson_result<simdjson::dom::object>> = true;
#endif // SIMDJSON_EXCEPTIONS
} // namespace ranges
} // namespace std
#endif // defined(__cpp_lib_ranges)

#endif // SIMDJSON_DOM_OBJECT_H
