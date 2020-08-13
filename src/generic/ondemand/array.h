#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * A forward-only JSON array.
 */
class array {
public:
  simdjson_really_inline array() noexcept;

  simdjson_really_inline array begin() noexcept;
  simdjson_really_inline array end() noexcept;

  //
  // Iterator interface
  //
  // Reads key and value, yielding them to the user.
  simdjson_really_inline simdjson_result<value> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  // Assumes it's being compared with the end. true if depth < doc->iter.depth.
  simdjson_really_inline bool operator==(const array &) noexcept;
  // Assumes it's being compared with the end. true if depth >= doc->iter.depth.
  simdjson_really_inline bool operator!=(const array &) noexcept;
  // Checks for ']' and ','
  simdjson_really_inline array &operator++() noexcept;

protected:
  /**
   * Begin array iteration.
   *
   * @param doc The document containing the array. The iterator must be just after the opening `[`.
   *            doc->iter.depth will be incremented automatically to reflect the nesting level.
   * @param error If this is not SUCCESS, creates an error chained array.
   */
  static simdjson_really_inline array begin(document *doc, error_code error=SUCCESS) noexcept;

  /**
   * Internal array creation. Call array::begin(doc[, error]) instead of this.
   *
   * @param doc The document containing the array. doc->iter.depth must already be incremented to
   *            reflect the array's depth. If there is no error, the iterator must be just after
   *            the opening `[`.
   * @param error The error to report. If the error is not SUCCESS, this is an error chained object.
   */
  simdjson_really_inline array(document *doc, error_code error) noexcept;

  /** Check whether iteration is complete. */
  bool finished() const noexcept;
  /** Decrements depth to mark iteration as complete. */
  void finish(bool log_end=false) noexcept;

  /**
   * Document containing this array.
   *
   * PERF NOTE: expected to be elided in favor of the parent document: this is set when the array
   * is first used, and never changes afterwards.
   */
  document *doc{};
  /**
   * Depth of the array.
   *
   * If doc->iter.depth < json.depth, we have finished.
   *
   * PERF NOTE: expected to be elided entirely, as any individual array's depth is a constant
   * knowable at compile time, incremented each time we nest an object or array.
   */
  uint32_t depth{};
  /**
   * Whether we're at the beginning of the array, or after.
   *
   * PERF NOTE: expected to be elided into inline control flow, as it is true for the first
   * iteration and false thereafter, and compilers with SSA optimization tend to analyze the first
   * iteration of any loop separately.
   */
  bool at_start{};
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
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::document *doc, error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) noexcept;

  simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::array begin() noexcept;
  simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::array end() noexcept;
};

} // namespace simdjson
