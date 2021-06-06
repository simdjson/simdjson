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
  simdjson_really_inline array() noexcept = default;

  /**
   * Begin array iteration.
   *
   * Part of the std::iterable interface.
   */
  simdjson_really_inline simdjson_result<array_iterator> begin() noexcept;
  /**
   * Sentinel representing the end of the array.
   *
   * Part of the std::iterable interface.
   */
  simdjson_really_inline simdjson_result<array_iterator> end() noexcept;
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
   */
  simdjson_really_inline simdjson_result<size_t> count_elements() & noexcept;
protected:
  /**
   * Begin array iteration.
   *
   * @param iter The iterator. Must be where the initial [ is expected. Will be *moved* into the
   *        resulting array.
   * @error INCORRECT_TYPE if the iterator is not at [.
   */
  static simdjson_really_inline simdjson_result<array> start(value_iterator &iter) noexcept;
  /**
   * Begin array iteration from the root.
   *
   * @param iter The iterator. Must be where the initial [ is expected. Will be *moved* into the
   *        resulting array.
   * @error INCORRECT_TYPE if the iterator is not at [.
   * @error TAPE_ERROR if there is no closing ] at the end of the document.
   */
  static simdjson_really_inline simdjson_result<array> start_root(value_iterator &iter) noexcept;
  /**
   * Begin array iteration.
   *
   * This version of the method should be called after the initial [ has been verified, and is
   * intended for use by switch statements that check the type of a value.
   *
   * @param iter The iterator. Must be after the initial [. Will be *moved* into the resulting array.
   */
  static simdjson_really_inline array started(value_iterator &iter) noexcept;

  /**
   * Create an array at the given Internal array creation. Call array::start() or array::started() instead of this.
   *
   * @param iter The iterator. Must either be at the start of the first element with iter.is_alive()
   *        == true, or past the [] with is_alive() == false if the array is empty. Will be *moved*
   *        into the resulting array.
   */
  simdjson_really_inline array(const value_iterator &iter) noexcept;

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
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::array &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> begin() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> end() noexcept;
  simdjson_really_inline simdjson_result<size_t> count_elements() & noexcept;
};

} // namespace simdjson
