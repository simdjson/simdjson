namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class document;
class object;
class array;
class value;
class raw_json_string;
class parser;
class json_iterator_ref;

/**
 * Iterates through JSON, with structure-sensitive algorithms.
 * 
 * @private This is not intended for external use.
 */
class json_iterator : public token_iterator {
public:
  simdjson_really_inline json_iterator() noexcept = default;
  simdjson_really_inline json_iterator(json_iterator &&other) noexcept;
  simdjson_really_inline json_iterator &operator=(json_iterator &&other) noexcept;
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  simdjson_really_inline ~json_iterator() noexcept;
#else
  simdjson_really_inline ~json_iterator() noexcept = default;
#endif
  simdjson_really_inline json_iterator(const json_iterator &other) noexcept = delete;
  simdjson_really_inline json_iterator &operator=(const json_iterator &other) noexcept = delete;

  /**
   * Check for an opening { and start an object iteration.
   *
   * @param json A pointer to the potential {
   * @returns Whether the object had any fields (returns false for empty).
   * @error INCORRECT_TYPE if there is no opening {
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> start_object(const uint8_t *json) noexcept;
  /**
   * Check for an opening { and start an object iteration.
   *
   * @returns Whether the object had any fields (returns false for empty).
   * @error INCORRECT_TYPE if there is no opening {
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> start_object() noexcept;

  /**
   * Start an object iteration after the user has already checked and moved past the {.
   *
   * Does not move the iterator.
   * 
   * @returns Whether the object had any fields (returns false for empty).
   */
  simdjson_warn_unused simdjson_really_inline bool started_object() noexcept;

  /**
   * Moves to the next field in an object.
   * 
   * Looks for , and }. If } is found, the object is finished and the iterator advances past it.
   * Otherwise, it advances to the next value.
   * 
   * @return whether there is another field in the object.
   * @error TAPE_ERROR If there is a comma missing between fields.
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> has_next_field() noexcept;

  /**
   * Get the current field's key.
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> field_key() noexcept;

  /**
   * Pass the : in the field and move to its value.
   */
  simdjson_warn_unused simdjson_really_inline error_code field_value() noexcept;

  /**
   * Find the next field with the given key.
   *
   * Assumes you have called next_field() or otherwise matched the previous value.
   * 
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> find_field_raw(const char *key) noexcept;

  /**
   * Check for an opening [ and start an array iteration.
   *
   * @param json A pointer to the potential [.
   * @returns Whether the array had any elements (returns false for empty).
   * @error INCORRECT_TYPE If there is no [.
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> start_array(const uint8_t *json) noexcept;
  /**
   * Check for an opening [ and start an array iteration.
   *
   * @returns Whether the array had any elements (returns false for empty).
   * @error INCORRECT_TYPE If there is no [.
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> start_array() noexcept;

  /**
   * Start an array iteration after the user has already checked and moved past the [.
   *
   * Does not move the iterator.
   *
   * @returns Whether the array had any elements (returns false for empty).
   */
  simdjson_warn_unused simdjson_really_inline bool started_array() noexcept;

  /**
   * Moves to the next element in an array.
   * 
   * Looks for , and ]. If ] is found, the array is finished and the iterator advances past it.
   * Otherwise, it advances to the next value.
   * 
   * @return Whether there is another element in the array.
   * @error TAPE_ERROR If there is a comma missing between elements.
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> has_next_element() noexcept;

  simdjson_warn_unused simdjson_really_inline simdjson_result<std::string_view> parse_string(const uint8_t *json) noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<std::string_view> consume_string() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> parse_raw_json_string(const uint8_t *json) noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> consume_raw_json_string() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> parse_uint64(const uint8_t *json) noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> consume_uint64() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> parse_int64(const uint8_t *json) noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> consume_int64() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t *json) noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<double> consume_double() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> parse_bool(const uint8_t *json) noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> consume_bool() noexcept;
  simdjson_really_inline bool is_null(const uint8_t *json) noexcept;
  simdjson_really_inline bool is_null() noexcept;

  simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> parse_root_uint64(const uint8_t *json) noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> consume_root_uint64() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> parse_root_int64(const uint8_t *json) noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> consume_root_int64() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<double> parse_root_double(const uint8_t *json) noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<double> consume_root_double() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> parse_root_bool(const uint8_t *json) noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> consume_root_bool() noexcept;
  simdjson_really_inline bool root_is_null(const uint8_t *json) noexcept;
  simdjson_really_inline bool root_is_null() noexcept;

  /**
   * Skips a JSON value, whether it is a scalar, array or object.
   */
  simdjson_warn_unused simdjson_really_inline error_code skip() noexcept;

  /**
   * Skips to the end of a JSON object or array.
   * 
   * @return true if this was the end of an array, false if it was the end of an object.
   */
  simdjson_warn_unused simdjson_really_inline error_code skip_container() noexcept;

  /**
   * Tell whether the iterator is still at the start
   */
  simdjson_really_inline bool at_start() const noexcept;

  /**
   * Tell whether the iterator is at the EOF mark
   */
  simdjson_really_inline bool at_eof() const noexcept;

  /**
   * Tell whether the iterator is live (has not been moved).
   */
  simdjson_really_inline bool is_alive() const noexcept;

  /**
   * Report an error, preventing further iteration.
   * 
   * @param error The error to report. Must not be SUCCESS, UNINITIALIZED, INCORRECT_TYPE, or NO_SUCH_FIELD.
   * @param message An error message to report with the error.
   */
  simdjson_really_inline error_code report_error(error_code error, const char *message) noexcept;

  /**
   * Get the error (if any).
   */
  simdjson_really_inline error_code error() const noexcept;

protected:
  ondemand::parser *parser{};
  /**
   * Next free location in the string buffer.
   * 
   * Used by raw_json_string::unescape() to have a place to unescape strings to.
   */
  uint8_t *current_string_buf_loc{};
  /**
   * JSON error, if there is one.
   * 
   * INCORRECT_TYPE and NO_SUCH_FIELD are *not* stored here, ever.
   *
   * PERF NOTE: we *hope* this will be elided into control flow, as it is only used (a) in the first
   * iteration of the loop, or (b) for the final iteration after a missing comma is found in ++. If
   * this is not elided, we should make sure it's at least not using up a register. Failing that,
   * we should store it in document so there's only one of them.
   */
  error_code _error{};
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  uint32_t active_lease_depth{};
#endif

  simdjson_really_inline json_iterator(ondemand::parser *parser) noexcept;
  template<int N>
  simdjson_warn_unused simdjson_really_inline bool copy_to_buffer(const uint8_t *json, uint8_t (&buf)[N]) noexcept;

  simdjson_really_inline json_iterator_ref borrow() noexcept;

  friend class document;
  friend class object;
  friend class array;
  friend class value;
  friend class raw_json_string;
  friend class parser;
  friend class json_iterator_ref;
  friend simdjson_really_inline void logger::log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta) noexcept;
}; // json_iterator

class json_iterator_ref {
public:
  simdjson_really_inline json_iterator_ref() noexcept = default;
  simdjson_really_inline json_iterator_ref(json_iterator_ref &&other) noexcept;
  simdjson_really_inline json_iterator_ref &operator=(json_iterator_ref &&other) noexcept;

#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  simdjson_really_inline ~json_iterator_ref() noexcept;
#else
  simdjson_really_inline ~json_iterator_ref() noexcept = default;
#endif // SIMDJSON_ONDEMAND_SAFETY_RAILS

  simdjson_really_inline json_iterator_ref(const json_iterator_ref &other) noexcept = delete;
  simdjson_really_inline json_iterator_ref &operator=(const json_iterator_ref &other) noexcept = delete;

  simdjson_really_inline json_iterator_ref borrow() noexcept;
  simdjson_really_inline void release() noexcept;

  simdjson_really_inline json_iterator *operator->() noexcept;
  simdjson_really_inline json_iterator &operator*() noexcept;
  simdjson_really_inline const json_iterator &operator*() const noexcept;

  simdjson_really_inline bool is_alive() const noexcept;
  simdjson_really_inline bool is_active() const noexcept;

  simdjson_really_inline void assert_is_active() const noexcept;
  simdjson_really_inline void assert_is_not_active() const noexcept;

private:
  json_iterator *iter{};
#ifdef SIMDJSON_ONDEMAND_SAFETY_RAILS
  uint32_t lease_depth{};
  simdjson_really_inline json_iterator_ref(json_iterator *iter, uint32_t lease_depth) noexcept;
#else
  simdjson_really_inline json_iterator_ref(json_iterator *iter) noexcept;
#endif

  friend class json_iterator;
}; // class json_iterator_ref

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private
};

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator_ref> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator_ref> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator_ref &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator_ref> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private
};

} // namespace simdjson
