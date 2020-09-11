#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * A forward-only JSON object field iterator.
 */
class object {
public:
  /**
   * Create a new invalid object.
   * 
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline object() noexcept = default;

  simdjson_really_inline object(object &&other) noexcept = default;
  simdjson_really_inline object &operator=(object &&other) noexcept = default;
  object(const object &) = delete;
  object &operator=(const object &) = delete;

  simdjson_really_inline ~object() noexcept;

  simdjson_really_inline object_iterator begin() noexcept;
  simdjson_really_inline object_iterator end() noexcept;
  simdjson_really_inline simdjson_result<value> operator[](const std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<value> operator[](const std::string_view key) && noexcept;

protected:
  /**
   * Begin object iteration.
   *
   * @param doc The document containing the object. The iterator must be just after the opening `{`.
   * @param error If this is not SUCCESS, creates an error chained object.
   */
  static simdjson_really_inline simdjson_result<object> start(json_iterator_ref &&iter) noexcept;
  static simdjson_really_inline object started(json_iterator_ref &&iter) noexcept;

  /**
   * Internal object creation. Call object::begin(doc) instead of this.
   *
   * @param doc The document containing the object. doc->depth must already be incremented to
   *            reflect the object's depth. The iterator must be just after the opening `{`.
   */
  simdjson_really_inline object(json_iterator_ref &&_iter) noexcept;

  simdjson_really_inline error_code find_field(const std::string_view key) noexcept;

  /**
   * Document containing the primary iterator.
   *
   * PERF NOTE: expected to be elided in favor of the parent document: this is set when the object
   * is first used, and never changes afterwards.
   */
  json_iterator_ref iter{};
  /**
   * Whether we are at the start.
   * 
   * PERF NOTE: this should be elided into inline control flow: it is only used for the first []
   * or * call, and SSA optimizers commonly do first-iteration loop optimization.
   */
  bool at_start{};

  friend class value;
  friend class document;
  friend struct simdjson_result<object>;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::object &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> begin() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> end() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) && noexcept;
};

} // namespace simdjson
