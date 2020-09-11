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

  simdjson_really_inline object_iterator(const object_iterator &o) noexcept = default;
  simdjson_really_inline object_iterator &operator=(const object_iterator &o) noexcept = default;

  //
  // Iterator interface
  //

  // Reads key and value, yielding them to the user.
  // MUST ONLY BE CALLED ONCE PER ITERATION.
  simdjson_really_inline simdjson_result<field> operator*() noexcept;
  // Assumes it's being compared with the end. true if depth < iter->depth.
  simdjson_really_inline bool operator==(const object_iterator &) noexcept;
  // Assumes it's being compared with the end. true if depth >= iter->depth.
  simdjson_really_inline bool operator!=(const object_iterator &) noexcept;
  // Checks for ']' and ','
  simdjson_really_inline object_iterator &operator++() noexcept;
private:
  json_iterator_ref *iter{};
  /**
   * Error, if there is one. Errors are only yielded once.
   *
   * PERF NOTE: we *hope* this will be elided into control flow, as it is only used (a) in the first
   * iteration of the loop, or (b) for the final iteration after a missing comma is found in ++. If
   * this is not elided, we should make sure it's at least not using up a register. Failing that,
   * we should store it in document so there's only one of them.
   */
  error_code error{};
  simdjson_really_inline object_iterator(json_iterator_ref &iter) noexcept;
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
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private

  //
  // Iterator interface
  //

  // Reads key and value, yielding them to the user.
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  // Assumes it's being compared with the end. true if depth < iter->depth.
  simdjson_really_inline bool operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &) noexcept;
  // Assumes it's being compared with the end. true if depth >= iter->depth.
  simdjson_really_inline bool operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &) noexcept;
  // Checks for ']' and ','
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> &operator++() noexcept;
};

} // namespace simdjson
