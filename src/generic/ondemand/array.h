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
   * @param doc The document containing the array.
   * @error INCORRECT_TYPE if the iterator is not at [.
   */
  static simdjson_really_inline array start(document *doc) noexcept;
  /**
   * Begin array iteration.
   *
   * @param doc The document containing the array. The iterator must be just after the opening `[`.
   */
  static simdjson_really_inline array started(document *doc) noexcept;
  /**
   * Created an error chained array iterator.
   *
   * @param doc The document containing the array.
   */
  static simdjson_really_inline array error_chain(document *doc, error_code error) noexcept;

  simdjson_really_inline error_code report_error() noexcept;

  /**
   * Internal array creation. Call array::start() or array::started() instead of this.
   *
   * @param doc The document containing the array. doc->iter.depth must already be incremented to
   *            reflect the array's depth. The iterator must be just after the opening `[`.
   * @param container The container returned from iter.start_array() / iter.started_array().
   */
  simdjson_really_inline array(document *_doc, json_iterator::container _container) noexcept;
  /**
   * Internal array creation. Call array::error_chain() instead of this.
   *
   * @param doc The document containing the array.
   * @param error The error to report. If it is not SUCCESS, this is an error chained object.
   */
  simdjson_really_inline array(document *_doc, error_code error) noexcept;

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
   * Container value for this array, obtained from json_iterator::started_array().
   *
   * PERF NOTE: expected to be elided entirely, as this is a constant knowable at compile time.
   */
  json_iterator::container container{};
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
