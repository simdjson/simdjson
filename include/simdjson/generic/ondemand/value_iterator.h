namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class document;
class object;
class array;
class value;
class raw_json_string;
class parser;

/**
 * Iterates through a single JSON value at a particular depth.
 *
 * Does not keep track of the type of value: provides methods for objects, arrays and scalars and expects
 * the caller to call the right ones.
 *
 * @private This is not intended for external use.
 */
class value_iterator {
protected:
  /** The underlying JSON iterator */
  json_iterator *_json_iter{};
  /** The depth of this value */
  depth_t _depth{};
  /**
   * The starting token index for this value
   */
  token_position _start_position{};

public:
  simdjson_inline value_iterator() noexcept = default;

  /**
   * Denote that we're starting a document.
   */
  simdjson_inline void start_document() noexcept;

  /**
   * Skips a non-iterated or partially-iterated JSON value, whether it is a scalar, array or object.
   *
   * Optimized for scalars.
   */
  simdjson_warn_unused simdjson_inline error_code skip_child() noexcept;

  /**
   * Tell whether the iterator is at the EOF mark
   */
  simdjson_inline bool at_end() const noexcept;

  /**
   * Tell whether the iterator is at the start of the value
   */
  simdjson_inline bool at_start() const noexcept;

  /**
   * Tell whether the value is open--if the value has not been used, or the array/object is still open.
   */
  simdjson_inline bool is_open() const noexcept;

  /**
   * Tell whether the value is at an object's first field (just after the {).
   */
  simdjson_inline bool at_first_field() const noexcept;

  /**
   * Abandon all iteration.
   */
  simdjson_inline void abandon() noexcept;

  /**
   * Get the child value as a value_iterator.
   */
  simdjson_inline value_iterator child_value() const noexcept;

  /**
   * Get the depth of this value.
   */
  simdjson_inline int32_t depth() const noexcept;

  /**
   * Get the JSON type of this value.
   *
   * @error TAPE_ERROR when the JSON value is a bad token like "}" "," or "alse".
   */
  simdjson_inline simdjson_result<json_type> type() const noexcept;

  /**
   * @addtogroup object Object iteration
   *
   * Methods to iterate and find object fields. These methods generally *assume* the value is
   * actually an object; the caller is responsible for keeping track of that fact.
   *
   * @{
   */

  /**
   * Start an object iteration.
   *
   * @returns Whether the object had any fields (returns false for empty).
   * @error INCORRECT_TYPE if there is no opening {
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> start_object() noexcept;
  /**
   * Start an object iteration from the root.
   *
   * @returns Whether the object had any fields (returns false for empty).
   * @error INCORRECT_TYPE if there is no opening {
   * @error TAPE_ERROR if there is no matching } at end of document
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> start_root_object() noexcept;

  /**
   * Start an object iteration after the user has already checked and moved past the {.
   *
   * Does not move the iterator unless the object is empty ({}).
   *
   * @returns Whether the object had any fields (returns false for empty).
   * @error INCOMPLETE_ARRAY_OR_OBJECT If there are no more tokens (implying the *parent*
   *        array or object is incomplete).
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> started_object() noexcept;
  /**
   * Start an object iteration from the root, after the user has already checked and moved past the {.
   *
   * Does not move the iterator unless the object is empty ({}).
   *
   * @returns Whether the object had any fields (returns false for empty).
   * @error INCOMPLETE_ARRAY_OR_OBJECT If there are no more tokens (implying the *parent*
   *        array or object is incomplete).
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> started_root_object() noexcept;

  /**
   * Moves to the next field in an object.
   *
   * Looks for , and }. If } is found, the object is finished and the iterator advances past it.
   * Otherwise, it advances to the next value.
   *
   * @return whether there is another field in the object.
   * @error TAPE_ERROR If there is a comma missing between fields.
   * @error TAPE_ERROR If there is a comma, but not enough tokens remaining to have a key, :, and value.
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> has_next_field() noexcept;

  /**
   * Get the current field's key.
   */
  simdjson_warn_unused simdjson_inline simdjson_result<raw_json_string> field_key() noexcept;

  /**
   * Pass the : in the field and move to its value.
   */
  simdjson_warn_unused simdjson_inline error_code field_value() noexcept;

  /**
   * Find the next field with the given key.
   *
   * Assumes you have called next_field() or otherwise matched the previous value.
   *
   * This means the iterator must be sitting at the next key:
   *
   * ```
   * { "a": 1, "b": 2 }
   *           ^
   * ```
   *
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  simdjson_warn_unused simdjson_inline error_code find_field(const std::string_view key) noexcept;

  /**
   * Find the next field with the given key, *without* unescaping. This assumes object order: it
   * will not find the field if it was already passed when looking for some *other* field.
   *
   * Assumes you have called next_field() or otherwise matched the previous value.
   *
   * This means the iterator must be sitting at the next key:
   *
   * ```
   * { "a": 1, "b": 2 }
   *           ^
   * ```
   *
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> find_field_raw(const std::string_view key) noexcept;

  /**
   * Find the field with the given key without regard to order, and *without* unescaping.
   *
   * This is an unordered object lookup: if the field is not found initially, it will cycle around and scan from the beginning.
   *
   * Assumes you have called next_field() or otherwise matched the previous value.
   *
   * This means the iterator must be sitting at the next key:
   *
   * ```
   * { "a": 1, "b": 2 }
   *           ^
   * ```
   *
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> find_field_unordered_raw(const std::string_view key) noexcept;

  /** @} */

  /**
   * @addtogroup array Array iteration
   * Methods to iterate over array elements. These methods generally *assume* the value is actually
   * an object; the caller is responsible for keeping track of that fact.
   * @{
   */

  /**
   * Check for an opening [ and start an array iteration.
   *
   * @returns Whether the array had any elements (returns false for empty).
   * @error INCORRECT_TYPE If there is no [.
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> start_array() noexcept;
  /**
   * Check for an opening [ and start an array iteration while at the root.
   *
   * @returns Whether the array had any elements (returns false for empty).
   * @error INCORRECT_TYPE If there is no [.
   * @error TAPE_ERROR if there is no matching ] at end of document
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> start_root_array() noexcept;

  /**
   * Start an array iteration, after the user has already checked and moved past the [.
   *
   * Does not move the iterator unless the array is empty ([]).
   *
   * @returns Whether the array had any elements (returns false for empty).
   * @error INCOMPLETE_ARRAY_OR_OBJECT If there are no more tokens (implying the *parent*
   *        array or object is incomplete).
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> started_array() noexcept;
  /**
   * Start an array iteration from the root, after the user has already checked and moved past the [.
   *
   * Does not move the iterator unless the array is empty ([]).
   *
   * @returns Whether the array had any elements (returns false for empty).
   * @error INCOMPLETE_ARRAY_OR_OBJECT If there are no more tokens (implying the *parent*
   *        array or object is incomplete).
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> started_root_array() noexcept;

  /**
   * Moves to the next element in an array.
   *
   * Looks for , and ]. If ] is found, the array is finished and the iterator advances past it.
   * Otherwise, it advances to the next value.
   *
   * @return Whether there is another element in the array.
   * @error TAPE_ERROR If there is a comma missing between elements.
   */
  simdjson_warn_unused simdjson_inline simdjson_result<bool> has_next_element() noexcept;

  /**
   * Get a child value iterator.
   */
  simdjson_warn_unused simdjson_inline value_iterator child() const noexcept;

  /** @} */

  /**
   * @defgroup scalar Scalar values
   * @addtogroup scalar
   * @{
   */

  simdjson_warn_unused simdjson_inline simdjson_result<std::string_view> get_string() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<uint64_t> get_uint64_in_string() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<int64_t> get_int64_in_string() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<double> get_double() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<double> get_double_in_string() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_inline bool is_null() noexcept;
  simdjson_warn_unused simdjson_inline bool is_negative() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<bool> is_integer() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<number_type> get_number_type() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<number> get_number() noexcept;

  simdjson_warn_unused simdjson_inline simdjson_result<std::string_view> get_root_string() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<raw_json_string> get_root_raw_json_string() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<uint64_t> get_root_uint64() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<uint64_t> get_root_uint64_in_string() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<int64_t> get_root_int64() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<int64_t> get_root_int64_in_string() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<double> get_root_double() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<double> get_root_double_in_string() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<bool> get_root_bool() noexcept;
  simdjson_warn_unused simdjson_inline bool is_root_negative() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<bool> is_root_integer() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<number_type> get_root_number_type() noexcept;
  simdjson_warn_unused simdjson_inline simdjson_result<number> get_root_number() noexcept;
  simdjson_inline bool is_root_null() noexcept;

  simdjson_inline error_code error() const noexcept;
  simdjson_inline uint8_t *&string_buf_loc() noexcept;
  simdjson_inline const json_iterator &json_iter() const noexcept;
  simdjson_inline json_iterator &json_iter() noexcept;

  simdjson_inline void assert_is_valid() const noexcept;
  simdjson_inline bool is_valid() const noexcept;

  /** @} */
protected:
  /**
   * Restarts an array iteration.
   * @returns Whether the array has any elements (returns false for empty).
   */
  simdjson_inline simdjson_result<bool> reset_array() noexcept;
  /**
   * Restarts an object iteration.
   * @returns Whether the object has any fields (returns false for empty).
   */
  simdjson_inline simdjson_result<bool> reset_object() noexcept;
  /**
   * move_at_start(): moves us so that we are pointing at the beginning of
   * the container. It updates the index so that at_start() is true and it
   * syncs the depth. The user can then create a new container instance.
   *
   * Usage: used with value::count_elements().
   **/
  simdjson_inline void move_at_start() noexcept;

  /**
   * move_at_container_start(): moves us so that we are pointing at the beginning of
   * the container so that assert_at_container_start() passes.
   *
   * Usage: used with reset_array() and reset_object().
   **/
   simdjson_inline void move_at_container_start() noexcept;
  /* Useful for debugging and logging purposes. */
  inline std::string to_string() const noexcept;
  simdjson_inline value_iterator(json_iterator *json_iter, depth_t depth, token_position start_index) noexcept;

  simdjson_inline bool parse_null(const uint8_t *json) const noexcept;
  simdjson_inline simdjson_result<bool> parse_bool(const uint8_t *json) const noexcept;
  simdjson_inline const uint8_t *peek_start() const noexcept;
  simdjson_inline uint32_t peek_start_length() const noexcept;

  /**
   * The general idea of the advance_... methods and the peek_* methods
   * is that you first peek and check that you have desired type. If you do,
   * and only if you do, then you advance.
   *
   * We used to unconditionally advance. But this made reasoning about our
   * current state difficult.
   * Suppose you always advance. Look at the 'value' matching the key
   * "shadowable" in the following example...
   *
   * ({"globals":{"a":{"shadowable":[}}}})
   *
   * If the user thinks it is a Boolean and asks for it, then we check the '[',
   * decide it is not a Boolean, but still move into the next character ('}'). Now
   * we are left pointing at '}' right after a '['. And we have not yet reported
   * an error, only that we do not have a Boolean.
   *
   * If, instead, you just stand your ground until it is content that you know, then
   * you will only even move beyond the '[' if the user tells you that you have an
   * array. So you will be at the '}' character inside the array and, hopefully, you
   * will then catch the error because an array cannot start with '}', but the code
   * processing Boolean values does not know this.
   *
   * So the contract is: first call 'peek_...' and then call 'advance_...' only
   * if you have determined that it is a type you can handle.
   *
   * Unfortunately, it makes the code more verbose, longer and maybe more error prone.
   */

  simdjson_inline void advance_scalar(const char *type) noexcept;
  simdjson_inline void advance_root_scalar(const char *type) noexcept;
  simdjson_inline void advance_non_root_scalar(const char *type) noexcept;

  simdjson_inline const uint8_t *peek_scalar(const char *type) noexcept;
  simdjson_inline const uint8_t *peek_root_scalar(const char *type) noexcept;
  simdjson_inline const uint8_t *peek_non_root_scalar(const char *type) noexcept;


  simdjson_inline error_code start_container(uint8_t start_char, const char *incorrect_type_message, const char *type) noexcept;
  simdjson_inline error_code end_container() noexcept;

  /**
   * Advance to a place expecting a value (increasing depth).
   *
   * @return The current token (the one left behind).
   * @error TAPE_ERROR If the document ended early.
   */
  simdjson_inline simdjson_result<const uint8_t *> advance_to_value() noexcept;

  simdjson_inline error_code incorrect_type_error(const char *message) const noexcept;
  simdjson_inline error_code error_unless_more_tokens(uint32_t tokens=1) const noexcept;

  simdjson_inline bool is_at_start() const noexcept;
  /**
   * is_at_iterator_start() returns true on an array or object after it has just been
   * created, whether the instance is empty or not.
   *
   * Usage: used by array::begin() in debug mode (SIMDJSON_DEVELOPMENT_CHECKS)
   */
  simdjson_inline bool is_at_iterator_start() const noexcept;

  /**
   * Assuming that we are within an object, this returns true if we
   * are pointing at a key.
   *
   * Usage: the skip_child() method should never be used while we are pointing
   * at a key inside an object.
   */
  simdjson_inline bool is_at_key() const noexcept;

  inline void assert_at_start() const noexcept;
  inline void assert_at_container_start() const noexcept;
  inline void assert_at_root() const noexcept;
  inline void assert_at_child() const noexcept;
  inline void assert_at_next() const noexcept;
  inline void assert_at_non_root_start() const noexcept;

  /** Get the starting position of this value */
  simdjson_inline token_position start_position() const noexcept;

  /** @copydoc error_code json_iterator::position() const noexcept; */
  simdjson_inline token_position position() const noexcept;
  /** @copydoc error_code json_iterator::end_position() const noexcept; */
  simdjson_inline token_position last_position() const noexcept;
  /** @copydoc error_code json_iterator::end_position() const noexcept; */
  simdjson_inline token_position end_position() const noexcept;
  /** @copydoc error_code json_iterator::report_error(error_code error, const char *message) noexcept; */
  simdjson_inline error_code report_error(error_code error, const char *message) noexcept;

  friend class document;
  friend class object;
  friend class array;
  friend class value;
}; // value_iterator

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::value_iterator &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_inline simdjson_result() noexcept = default;
};

} // namespace simdjson
