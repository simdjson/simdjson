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
  simdjson_really_inline array(array &&other) noexcept = default;
  simdjson_really_inline array &operator=(array &&other) noexcept = default;
  array(const array &) = delete;
  array &operator=(const array &) = delete;

  /**
   * Finishes iterating the array if it is not already fully iterated.
   */
  simdjson_really_inline ~array() noexcept;

  /**
   * Begin array iteration.
   *
   * Part of the std::iterable interface.
   */
  simdjson_really_inline array_iterator<array> begin() & noexcept;
  /**
   * Sentinel representing the end of the array.
   *
   * Part of the std::iterable interface.
   */
  simdjson_really_inline array_iterator<array> end() & noexcept;

protected:
  /**
   * Begin array iteration.
   *
   * @param iter The iterator. Must be where the initial [ is expected. Will be *moved* into the
   *        resulting array.
   * @error INCORRECT_TYPE if the iterator is not at [.
   */
  static simdjson_really_inline simdjson_result<array> start(json_iterator_ref &&iter) noexcept;
  /**
   * Begin array iteration.
   * 
   * This version of the method should be called after the initial [ has been verified, and is
   * intended for use by switch statements that check the type of a value.
   *
   * @param iter The iterator. Must be after the initial [. Will be *moved* into the resulting array.
   */
  static simdjson_really_inline array started(json_iterator_ref &&iter) noexcept;

  /**
   * Create an array at the given Internal array creation. Call array::start() or array::started() instead of this.
   *
   * @param iter The iterator. Must either be at the start of the first element with iter.is_alive()
   *        == true, or past the [] with is_alive() == false if the array is empty. Will be *moved*
   *        into the resulting array.
   */
  simdjson_really_inline array(json_iterator_ref &&iter) noexcept;

  //
  // For array_iterator
  //
  simdjson_really_inline json_iterator &get_iterator() noexcept;
  simdjson_really_inline json_iterator_ref borrow_iterator() noexcept;
  simdjson_really_inline bool is_iterator_alive() const noexcept;
  simdjson_really_inline void iteration_finished() noexcept;

  /**
   * Iterator marking current position.
   * 
   * iter.is_alive() == false indicates iteration is complete.
   */
  json_iterator_ref iter{};

  friend class value;
  friend struct simdjson_result<value>;
  friend struct simdjson_result<array>;
  friend class array_iterator<array>;
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
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<SIMDJSON_IMPLEMENTATION::ondemand::array>> begin() & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<SIMDJSON_IMPLEMENTATION::ondemand::array>> end() & noexcept;
};

} // namespace simdjson
