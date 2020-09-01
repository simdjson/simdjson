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
  simdjson_really_inline array() noexcept;
  simdjson_really_inline ~array() noexcept;
  simdjson_really_inline array(array &&other) noexcept;
  simdjson_really_inline array &operator=(array &&other) noexcept;
  array(const array &) = delete;
  array &operator=(const array &) = delete;

  class iterator {
  public:
    simdjson_really_inline iterator(array &a) noexcept;
    simdjson_really_inline iterator(const array::iterator &a) noexcept;
    simdjson_really_inline iterator &operator=(const array::iterator &a) noexcept;

    //
    // Iterator interface
    //

    // Reads key and value, yielding them to the user.
    simdjson_really_inline simdjson_result<value> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
    // Assumes it's being compared with the end. true if depth < iter->depth.
    simdjson_really_inline bool operator==(const array::iterator &) noexcept;
    // Assumes it's being compared with the end. true if depth >= iter->depth.
    simdjson_really_inline bool operator!=(const array::iterator &) noexcept;
    // Checks for ']' and ','
    simdjson_really_inline array::iterator &operator++() noexcept;
  private:
    array *a{};
    simdjson_really_inline iterator() noexcept;
    friend struct simdjson_result<array::iterator>;
  };

  simdjson_really_inline array::iterator begin() noexcept;
  simdjson_really_inline array::iterator end() noexcept;

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
  /**
   * Error, if there is one. Errors are only yielded once.
   *
   * PERF NOTE: we *hope* this will be elided into control flow, as it is only used (a) in the first
   * iteration of the loop, or (b) for the final iteration after a missing comma is found in ++. If
   * this is not elided, we should make sure it's at least not using up a register. Failing that,
   * we should store it in document so there's only one of them.
   */
  error_code error{};

  friend class value;
  friend struct simdjson_result<value>;
  friend struct simdjson_result<array>;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> : public internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::array &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> begin() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> end() noexcept;
};

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> : public internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> {
public:
  simdjson_really_inline simdjson_result() noexcept;
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::array::iterator &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &a) noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &operator=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &a) noexcept;

  //
  // Iterator interface
  //

  // Reads key and value, yielding them to the user.
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  // Assumes it's being compared with the end. true if depth < iter->depth.
  simdjson_really_inline bool operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &) noexcept;
  // Assumes it's being compared with the end. true if depth >= iter->depth.
  simdjson_really_inline bool operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &) noexcept;
  // Checks for ']' and ','
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> &operator++() noexcept;
};

} // namespace simdjson
