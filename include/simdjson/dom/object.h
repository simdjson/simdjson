#ifndef SIMDJSON_DOM_OBJECT_H
#define SIMDJSON_DOM_OBJECT_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/tape_ref.h"
#include "simdjson/minify.h"
#include <ostream>

namespace simdjson {
namespace dom {

class document;
class element;
class key_value_pair;

/**
 * JSON object.
 */
class object {
public:
  /** Create a new, invalid object */
  really_inline object() noexcept;

  class iterator {
  public:
    /**
     * Get the actual key/value pair
     */
    inline const key_value_pair operator*() const noexcept;
    /**
     * Get the next key/value pair.
     *
     * Part of the std::iterator interface.
     *
     */
    inline iterator& operator++() noexcept;
    /**
     * Check if these key value pairs come from the same place in the JSON.
     *
     * Part of the std::iterator interface.
     */
    inline bool operator!=(const iterator& other) const noexcept;
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
    inline bool key_equals(const std::string_view & o) const noexcept;
    /**
     * Returns true if the key in this key/value pair is equal
     * to the provided string_view in a case-insensitive manner.
     * Case comparisons may only be handled correctly for ASCII strings.
     */
    inline bool key_equals_case_insensitive(const std::string_view & o) const noexcept;
    /**
     * Get the key of this key/value pair.
     */
    inline const char *key_c_str() const noexcept;
    /**
     * Get the value of this key/value pair.
     */
    inline element value() const noexcept;
  private:
    really_inline iterator(const internal::tape_ref &tape) noexcept;

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
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\n"].get<uint64_t>().first == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get<uint64_t>().error() == NO_SUCH_FIELD
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](const std::string_view &key) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\n"].get<uint64_t>().first == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get<uint64_t>().error() == NO_SUCH_FIELD
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](const char *key) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   dom::parser parser;
   *   object obj = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})"_padded);
   *   obj.at("foo/a/1") == 20
   *   obj.at("foo")["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<element> at(const std::string_view &json_pointer) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\n"].get<uint64_t>().first == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get<uint64_t>().error() == NO_SUCH_FIELD
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key(const std::string_view &key) const noexcept;

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
  inline simdjson_result<element> at_key_case_insensitive(const std::string_view &key) const noexcept;

private:
  really_inline object(const internal::tape_ref &tape) noexcept;

  internal::tape_ref tape;

  friend class element;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::minifier;
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
  really_inline key_value_pair(const std::string_view &_key, element _value) noexcept;
  friend class object;
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
inline std::ostream& operator<<(std::ostream& out, const object &value);
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const key_value_pair &value);

} // namespace dom

/** The result of a JSON conversion that may fail. */
template<>
struct simdjson_result<dom::object> : public internal::simdjson_result_base<dom::object> {
public:
  really_inline simdjson_result() noexcept; ///< @private
  really_inline simdjson_result(dom::object value) noexcept; ///< @private
  really_inline simdjson_result(error_code error) noexcept; ///< @private

  inline simdjson_result<dom::element> operator[](const std::string_view &key) const noexcept;
  inline simdjson_result<dom::element> operator[](const char *key) const noexcept;
  inline simdjson_result<dom::element> at(const std::string_view &json_pointer) const noexcept;
  inline simdjson_result<dom::element> at_key(const std::string_view &key) const noexcept;
  inline simdjson_result<dom::element> at_key_case_insensitive(const std::string_view &key) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline dom::object::iterator begin() const noexcept(false);
  inline dom::object::iterator end() const noexcept(false);
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
inline std::ostream& operator<<(std::ostream& out, const simdjson_result<dom::object> &value) noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

#endif // SIMDJSON_DOM_OBJECT_H
