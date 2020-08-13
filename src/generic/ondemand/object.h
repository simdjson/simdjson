#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * A forward-only JSON object field iterator.
 */
class object {
public:
  simdjson_really_inline object() noexcept;

  simdjson_really_inline object begin() noexcept;
  simdjson_really_inline object end() noexcept;
  simdjson_really_inline simdjson_result<value> operator[](const std::string_view key) noexcept;

  //
  // Iterator interface
  //
  // Reads key and value, yielding them to the user.
  simdjson_really_inline simdjson_result<field> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  // Assumes it's being compared with the end. true if depth < doc->iter.depth.
  simdjson_really_inline bool operator==(const object &) noexcept;
  // Assumes it's being compared with the end. true if depth >= doc->iter.depth.
  simdjson_really_inline bool operator!=(const object &) noexcept;
  // Checks for '}' and ','
  simdjson_really_inline object &operator++() noexcept;

protected:
  /**
   * Begin object iteration.
   *
   * @param doc The document containing the object. The iterator must be just after the opening `{`.
   * @param error If this is not SUCCESS, creates an error chained object.
   */
  static simdjson_really_inline object begin(document *doc, error_code error=SUCCESS) noexcept;

  /**
   * Internal object creation. Call object::begin(doc[, error]) instead of this.
   *
   * @param doc The document containing the object. doc->depth must already be incremented to
   *            reflect the object's depth. If there is no error, the iterator must be just after
   *            the opening `{`.
   * @param error The error to report. If the error is not SUCCESS, this is an error chained object.
   */
  simdjson_really_inline object(document *doc, error_code error) noexcept;

  /** Check whether iteration is complete. */
  simdjson_really_inline bool finished() const noexcept;
  /** Decrements depth to mark iteration as complete. */
  simdjson_really_inline void finish(bool log_end=false) noexcept;
  simdjson_really_inline void first_field() noexcept;

  /**
   * Document containing the primary iterator.
   *
   * PERF NOTE: expected to be elided in favor of the parent document: this is set when the object
   * is first used, and never changes afterwards.
   */
  document *doc{};
  /**
   * Depth of the object.
   *
   * If doc->iter.depth < json.depth, we have finished.
   *
   * PERF NOTE: expected to be elided entirely, as any individual object's depth is a constant
   * knowable at compile time, incremented each time we nest an object() or array().
   */
  uint32_t depth{};
  /**
   * Whether we're at the beginning of the object, or after.
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
  friend class document;
  friend struct simdjson_result<object>;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> : public internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::object &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::document *doc, error_code error) noexcept; ///< @private

  simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::object begin() noexcept;
  simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::object end() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) noexcept;
};

} // namespace simdjson
