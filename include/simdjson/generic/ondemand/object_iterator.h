#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class field;

class object_iterator {
public:
  /**
   * Create a new invalid object_iterator.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline object_iterator() noexcept = default;

  //
  // Iterator interface
  //

  // Reads key and value, yielding them to the user.
  // MUST ONLY BE CALLED ONCE PER ITERATION.
  simdjson_really_inline simdjson_result<field> operator*() noexcept;
  // Assumes it's being compared with the end. true if depth < iter->depth.
  simdjson_really_inline bool operator==(const object_iterator &) const noexcept;
  // Assumes it's being compared with the end. true if depth >= iter->depth.
  simdjson_really_inline bool operator!=(const object_iterator &) const noexcept;
  // Checks for ']' and ','
  simdjson_really_inline object_iterator &operator++() noexcept;

  /**
   * Find the field with the given key. May be used in place of ++.
   */
  simdjson_warn_unused simdjson_really_inline error_code find_field_raw(const std::string_view key) noexcept;

private:
  /**
   * The underlying JSON iterator.
   *
   * PERF NOTE: expected to be elided in favor of the parent document: this is set when the object
   * is first used, and never changes afterwards.
   */
  value_iterator iter{};
  /**
   * Whether we are at the start.
   *
   * PERF NOTE: this should be elided into inline control flow: it is only used for the first []
   * or * call, and SSA optimizers commonly do first-iteration loop optimization.
   *
   * SAFETY: this is not safe; the object_iterator can be copied freely, so the state CAN be lost.
   */
  bool at_start{};

  simdjson_really_inline object_iterator(const value_iterator &iter, bool at_start = true) noexcept;
  friend struct simdjson_result<object_iterator>;
  friend class object;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::object_iterator &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;

  //
  // Iterator interface
  //

  // Reads key and value, yielding them to the user.
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  // Assumes it's being compared with the end. true if depth < iter->depth.
  simdjson_really_inline bool operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &) const noexcept;
  // Assumes it's being compared with the end. true if depth >= iter->depth.
  simdjson_really_inline bool operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &) const noexcept;
  // Checks for ']' and ','
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &operator++() noexcept;
};

} // namespace simdjson
