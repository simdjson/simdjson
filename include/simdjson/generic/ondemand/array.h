#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class value;
class document;

/**
 * A forward-only JSON array.
 */
class array {
public:
  /**
   * Create a new invalid array.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_inline array() noexcept = default;

  /**
   * Begin array iteration.
   *
   * Part of the std::iterable interface.
   */
  simdjson_inline simdjson_result<array_iterator> begin() noexcept;
  /**
   * Sentinel representing the end of the array.
   *
   * Part of the std::iterable interface.
   */
  simdjson_inline simdjson_result<array_iterator> end() noexcept;
  /**
   * This method scans the array and counts the number of elements.
   * The count_elements method should always be called before you have begun
   * iterating through the array: it is expected that you are pointing at
   * the beginning of the array.
   * The runtime complexity is linear in the size of the array. After
   * calling this function, if successful, the array is 'rewinded' at its
   * beginning as if it had never been accessed. If the JSON is malformed (e.g.,
   * there is a missing comma), then an error is returned and it is no longer
   * safe to continue.
   *
   * To check that an array is empty, it is more performant to use
   * the is_empty() method.
   */
  simdjson_inline simdjson_result<size_t> count_elements() & noexcept;
  /**
   * This method scans the beginning of the array and checks whether the
   * array is empty.
   * The runtime complexity is constant time. After
   * calling this function, if successful, the array is 'rewinded' at its
   * beginning as if it had never been accessed. If the JSON is malformed (e.g.,
   * there is a missing comma), then an error is returned and it is no longer
   * safe to continue.
   */
  simdjson_inline simdjson_result<bool> is_empty() & noexcept;
  /**
   * Reset the iterator so that we are pointing back at the
   * beginning of the array. You should still consume values only once even if you
   * can iterate through the array more than once. If you unescape a string
   * within the array more than once, you have unsafe code. Note that rewinding
   * an array means that you may need to reparse it anew: it is not a free
   * operation.
   *
   * @returns true if the array contains some elements (not empty)
   */
  inline simdjson_result<bool> reset() & noexcept;
  /**
   * Get the value associated with the given JSON pointer.  We use the RFC 6901
   * https://tools.ietf.org/html/rfc6901 standard, interpreting the current node
   * as the root of its own JSON document.
   *
   *   ondemand::parser parser;
   *   auto json = R"([ { "foo": { "a": [ 10, 20, 30 ] }} ])"_padded;
   *   auto doc = parser.iterate(json);
   *   doc.at_pointer("/0/foo/a/1") == 20
   *
   * Note that at_pointer() called on the document automatically calls the document's rewind
   * method between each call. It invalidates all previously accessed arrays, objects and values
   * that have not been consumed. Yet it is not the case when calling at_pointer on an array
   * instance: there is no rewind and no invalidation.
   *
   * You may only call at_pointer on an array after it has been created, but before it has
   * been first accessed. When calling at_pointer on an array, the pointer is advanced to
   * the location indicated by the JSON pointer (in case of success). It is no longer possible
   * to call at_pointer on the same array.
   *
   * Also note that at_pointer() relies on find_field() which implies that we do not unescape keys when matching.
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<value> at_pointer(std::string_view json_pointer) noexcept;
  /**
   * Consumes the array and returns a string_view instance corresponding to the
   * array as represented in JSON. It points inside the original document.
   */
  simdjson_inline simdjson_result<std::string_view> raw_json() noexcept;

  /**
   * Get the value at the given index. This function has linear-time complexity.
   * This function should only be called once on an array instance since the array iterator is not reset between each call.
   *
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  simdjson_inline simdjson_result<value> at(size_t index) noexcept;
protected:
  /**
   * Go to the end of the array, no matter where you are right now.
   */
  simdjson_inline error_code consume() noexcept;

  /**
   * Begin array iteration.
   *
   * @param iter The iterator. Must be where the initial [ is expected. Will be *moved* into the
   *        resulting array.
   * @error INCORRECT_TYPE if the iterator is not at [.
   */
  static simdjson_inline simdjson_result<array> start(value_iterator &iter) noexcept;
  /**
   * Begin array iteration from the root.
   *
   * @param iter The iterator. Must be where the initial [ is expected. Will be *moved* into the
   *        resulting array.
   * @error INCORRECT_TYPE if the iterator is not at [.
   * @error TAPE_ERROR if there is no closing ] at the end of the document.
   */
  static simdjson_inline simdjson_result<array> start_root(value_iterator &iter) noexcept;
  /**
   * Begin array iteration.
   *
   * This version of the method should be called after the initial [ has been verified, and is
   * intended for use by switch statements that check the type of a value.
   *
   * @param iter The iterator. Must be after the initial [. Will be *moved* into the resulting array.
   */
  static simdjson_inline simdjson_result<array> started(value_iterator &iter) noexcept;

  /**
   * Create an array at the given Internal array creation. Call array::start() or array::started() instead of this.
   *
   * @param iter The iterator. Must either be at the start of the first element with iter.is_alive()
   *        == true, or past the [] with is_alive() == false if the array is empty. Will be *moved*
   *        into the resulting array.
   */
  simdjson_inline array(const value_iterator &iter) noexcept;

  /**
   * Iterator marking current position.
   *
   * iter.is_alive() == false indicates iteration is complete.
   */
  value_iterator iter{};

  friend class value;
  friend class document;
  friend struct simdjson_result<value>;
  friend struct simdjson_result<array>;
  friend class array_iterator;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::array &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_inline simdjson_result() noexcept = default;

  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> begin() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> end() noexcept;
  inline simdjson_result<size_t> count_elements() & noexcept;
  inline simdjson_result<bool> is_empty() & noexcept;
  inline simdjson_result<bool> reset() & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> at(size_t index) noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> at_pointer(std::string_view json_pointer) noexcept;
};

} // namespace simdjson
