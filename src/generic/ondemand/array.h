#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class value;
class document;

/**
 * A forward-only JSON array.
 */
class array {
public:
  simdjson_really_inline array() noexcept = default;
  simdjson_really_inline array(array &&other) noexcept = default;
  simdjson_really_inline array &operator=(array &&other) noexcept = default;
  array(const array &) = delete;
  array &operator=(const array &) = delete;

  simdjson_really_inline ~array() noexcept;

  simdjson_really_inline array_iterator begin() & noexcept;
  simdjson_really_inline array_iterator end() & noexcept;

protected:
  /**
   * Begin array iteration.
   *
   * @param doc The document containing the array.
   * @error INCORRECT_TYPE if the iterator is not at [.
   */
  static simdjson_really_inline simdjson_result<array> start(json_iterator_ref &&iter) noexcept;
  /**
   * Begin array iteration.
   *
   * @param doc The document containing the array. The iterator must be just after the opening `[`.
   */
  static simdjson_really_inline array started(json_iterator_ref &&iter) noexcept;

  /**
   * Internal array creation. Call array::start() or array::started() instead of this.
   *
   * @param doc The document containing the array. iter->depth must already be incremented to
   *            reflect the array's depth. The iterator must be just after the opening `[`.
   */
  simdjson_really_inline array(json_iterator_ref &&iter) noexcept;

  /**
   * Document containing this array.
   *
   * PERF NOTE: expected to be elided in favor of the parent document: this is set when the array
   * is first used, and never changes afterwards.
   */
  json_iterator_ref iter{};

  friend class value;
  friend struct simdjson_result<value>;
  friend struct simdjson_result<array>;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::array &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> begin() & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> end() & noexcept;
};

} // namespace simdjson
