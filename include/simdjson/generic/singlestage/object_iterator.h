#ifndef SIMDJSON_GENERIC_SINGLESTAGE_OBJECT_ITERATOR_H

#ifndef SIMDJSON_AMALGAMATED
#define SIMDJSON_GENERIC_SINGLESTAGE_OBJECT_ITERATOR_H
#include "simdjson/generic/singlestage/base.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/singlestage/value_iterator.h"
#endif // SIMDJSON_AMALGAMATED

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace singlestage {

class object_iterator {
public:
  /**
   * Create a new invalid object_iterator.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_inline object_iterator() noexcept = default;

  //
  // Iterator interface
  //

  // Reads key and value, yielding them to the user.
  // MUST ONLY BE CALLED ONCE PER ITERATION.
  simdjson_inline simdjson_result<field> operator*() noexcept;
  // Assumes it's being compared with the end. true if depth < iter->depth.
  simdjson_inline bool operator==(const object_iterator &) const noexcept;
  // Assumes it's being compared with the end. true if depth >= iter->depth.
  simdjson_inline bool operator!=(const object_iterator &) const noexcept;
  // Checks for ']' and ','
  simdjson_inline object_iterator &operator++() noexcept;

private:
  /**
   * The underlying JSON iterator.
   *
   * PERF NOTE: expected to be elided in favor of the parent document: this is set when the object
   * is first used, and never changes afterwards.
   */
  value_iterator iter{};

  simdjson_inline object_iterator(const value_iterator &iter) noexcept;
  friend struct simdjson_result<object_iterator>;
  friend class object;
};

} // namespace singlestage
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::object_iterator> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::singlestage::object_iterator> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::singlestage::object_iterator &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_inline simdjson_result() noexcept = default;

  //
  // Iterator interface
  //

  // Reads key and value, yielding them to the user.
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::field> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  // Assumes it's being compared with the end. true if depth < iter->depth.
  simdjson_inline bool operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::object_iterator> &) const noexcept;
  // Assumes it's being compared with the end. true if depth >= iter->depth.
  simdjson_inline bool operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::object_iterator> &) const noexcept;
  // Checks for ']' and ','
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::object_iterator> &operator++() noexcept;
};

} // namespace simdjson

#endif // SIMDJSON_GENERIC_SINGLESTAGE_OBJECT_ITERATOR_H