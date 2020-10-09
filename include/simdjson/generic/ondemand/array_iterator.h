#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class array;
class value;
class document;

/**
 * A forward-only JSON array.
 * 
 * This is an input_iterator, meaning:
 * - It is forward-only
 * - * must be called exactly once per element.
 * - ++ must be called exactly once in between each * (*, ++, *, ++, * ...)
 */
template<typename T>
class array_iterator {
public:
  /** Create a new, invalid array iterator. */
  simdjson_really_inline array_iterator() noexcept = default;
  simdjson_really_inline array_iterator(const array_iterator<T> &a) noexcept = default;
  simdjson_really_inline array_iterator<T> &operator=(const array_iterator<T> &a) noexcept = default;

  //
  // Iterator interface
  //

  /**
   * Get the current element.
   * 
   * Part of the std::iterator interface.
   */
  simdjson_really_inline simdjson_result<value> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  /**
   * Check if we are at the end of the JSON.
   *
   * Part of the std::iterator interface.
   * 
   * @return true if there are no more elements in the JSON array.
   */
  simdjson_really_inline bool operator==(const array_iterator<T> &) noexcept;
  /**
   * Check if there are more elements in the JSON array.
   *
   * Part of the std::iterator interface.
   * 
   * @return true if there are more elements in the JSON array.
   */
  simdjson_really_inline bool operator!=(const array_iterator<T> &) noexcept;
  /**
   * Move to the next element.
   * 
   * Part of the std::iterator interface.
   */
  simdjson_really_inline array_iterator<T> &operator++() noexcept;

private:
  T *iter{};

  simdjson_really_inline array_iterator(T &iter) noexcept;

  static simdjson_really_inline simdjson_result<array_iterator<T>> start(T &iter, const uint8_t *json) noexcept;

  friend T;
  friend class array;
  friend class value;
  friend struct simdjson_result<array_iterator<T>>;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<typename T>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T> &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private

  //
  // Iterator interface
  //

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  simdjson_really_inline bool operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>> &) noexcept;
  simdjson_really_inline bool operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>> &) noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<T>> &operator++() noexcept;
};

} // namespace simdjson
