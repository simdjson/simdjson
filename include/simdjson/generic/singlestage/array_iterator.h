#ifndef SIMDJSON_GENERIC_SINGLESTAGE_ARRAY_ITERATOR_H

#ifndef SIMDJSON_AMALGAMATED
#define SIMDJSON_GENERIC_SINGLESTAGE_ARRAY_ITERATOR_H
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/singlestage/base.h"
#include "simdjson/generic/singlestage/value_iterator.h"
#endif // SIMDJSON_AMALGAMATED


namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace singlestage {

/**
 * A forward-only JSON array.
 *
 * This is an input_iterator, meaning:
 * - It is forward-only
 * - * must be called exactly once per element.
 * - ++ must be called exactly once in between each * (*, ++, *, ++, * ...)
 */
class array_iterator {
public:
  /** Create a new, invalid array iterator. */
  simdjson_inline array_iterator() noexcept = default;

  //
  // Iterator interface
  //

  /**
   * Get the current element.
   *
   * Part of the std::iterator interface.
   */
  simdjson_inline simdjson_result<value> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  /**
   * Check if we are at the end of the JSON.
   *
   * Part of the std::iterator interface.
   *
   * @return true if there are no more elements in the JSON array.
   */
  simdjson_inline bool operator==(const array_iterator &) const noexcept;
  /**
   * Check if there are more elements in the JSON array.
   *
   * Part of the std::iterator interface.
   *
   * @return true if there are more elements in the JSON array.
   */
  simdjson_inline bool operator!=(const array_iterator &) const noexcept;
  /**
   * Move to the next element.
   *
   * Part of the std::iterator interface.
   */
  simdjson_inline array_iterator &operator++() noexcept;

private:
  value_iterator iter{};

  simdjson_inline array_iterator(const value_iterator &iter) noexcept;

  friend class array;
  friend class value;
  friend struct simdjson_result<array_iterator>;
};

} // namespace singlestage
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::array_iterator> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::singlestage::array_iterator> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::singlestage::array_iterator &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_inline simdjson_result() noexcept = default;

  //
  // Iterator interface
  //

  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::value> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  simdjson_inline bool operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::array_iterator> &) const noexcept;
  simdjson_inline bool operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::array_iterator> &) const noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::array_iterator> &operator++() noexcept;
};

} // namespace simdjson

#endif // SIMDJSON_GENERIC_SINGLESTAGE_ARRAY_ITERATOR_H