namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * Iterates through JSON, with structure-sensitive algorithms.
 * 
 * @private This is not intended for external use.
 */
class json_iterator : public token_iterator {
public:
  /**
   * Marker value for a container: this is a convenience feature that lets users skip deeply nested
   * values until they return to the container (or skip the container itself).
   */
  struct container {
  public:
    simdjson_really_inline container() noexcept : depth{} {}
    simdjson_really_inline container child() const noexcept { return depth+1; }
    simdjson_really_inline container parent() const noexcept { return depth-1; }
  private:
    simdjson_really_inline container(uint32_t _depth) noexcept : depth{_depth} {}
    uint32_t depth;
    friend class json_iterator;
  };
  simdjson_really_inline json_iterator() noexcept;
  simdjson_really_inline json_iterator(json_iterator &&other) noexcept;
  simdjson_really_inline json_iterator &operator=(json_iterator &&other) noexcept;
  simdjson_really_inline json_iterator(const json_iterator &other) noexcept = delete;
  simdjson_really_inline json_iterator &operator=(const json_iterator &other) noexcept = delete;

  /**
   * Check for an opening { and start an object iteration.
   *
   * @return the object in question.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<container> start_object() noexcept;

  /**
   * Start an object iteration after the user has already checked and moved past the {.
   *
   * Does not move the iterator.
   *
   * @return the object in question.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline container started_object() noexcept;

  /**
   * Checks for empty object.
   * 
   * Assumes it's just past the { and start_object/started_object has been called.
   *
   * Finishes the array if so, advancing past the ]. Otherwise, leaves the iterator on the
   * first value.
   * 
   * @return true if it's an empty object, false if it has fields.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline bool is_empty_object() noexcept;

  /**
   * Moves to the next field in an object.
   * 
   * Looks for , and }. If } is found, the object is finished and the iterator advances past it.
   * Otherwise, it advances to the next value.
   * 
   * @return whether there is another field in the object.
   * @error TAPE_ERROR If there is a comma missing between fields.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> has_next_field() noexcept;

  /**
   * Moves to the next field in an object, finishing up any unfinished children.
   * 
   * Roughly equivalent to:
   * 
   *   iter.skip_unfinished_children(c);
   *   iter.has_next_field();
   *
   * @return whether there is another field in the array.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> next_field(container c) noexcept;

  /**
   * Get the current field's key.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<raw_json_string> field_key() noexcept;

  /**
   * Pass the : in the field and move to its value.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code field_value() noexcept;

  /**
   * Find the next field with the given key.
   *
   * Assumes you have called next_field() or otherwise matched the previous value.
   * 
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> find_field_raw(const char *key) noexcept;

  /**
   * Find the first field with the given key. Assumes start_object() or started_object() was already
   * called.
   *
   * Equivalent to:
   * 
   *     first_field(key);
   *     find_field_raw(key)
   *
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> find_first_field_raw(const char *key) noexcept;

  /**
   * Start an object and move to a single field, by name.
   * 
   * For use when you don't intend to do anything else with the object.
   *
   * Equivalent to:
   * 
   *     start_object(key);
   *     first_field(key);
   *     find_field_raw(key)
   *
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> find_single_field_raw(const char *key) noexcept;

  /**
   * Find the next field with the given key.
   *
   * Equivalent to:
   * 
   *     next_field(key, container);
   *     find_field_raw(key)
   *
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> find_next_field_raw(const char *key, container c) noexcept;

  /**
   * Check for an opening { and start an object iteration.
   *
   * @return the object in question.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<container> start_array() noexcept;

  /**
   * Start an array iteration after the user has already checked and moved past the [.
   *
   * Does not move the iterator.
   *
   * @return the array in question.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline container started_array() noexcept;

  /**
   * Check for empty array.
   *
   * Assumes it's just past the [ and start_object/started_object has been called.
   *
   * Finishes the array if so, advancing past the ]. Otherwise, leaves the iterator on the
   * first value.
   * 
   * @return true if it is an empty array, false if not.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline bool is_empty_array() noexcept;

  /**
   * Moves to the next element in an array.
   * 
   * Looks for , and ]. If ] is found, the array is finished and the iterator advances past it.
   * Otherwise, it advances to the next value.
   * 
   * @return Whether there is another element in the array.
   * @error TAPE_ERROR If there is a comma missing between elements.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> has_next_element() noexcept;

  /**
   * Moves to the next element in an array, skipping any unfinished children first.
   * 
   * Roughly equivalent to:
   * 
   *   iter.skip_unfinished_children(c);
   *   iter.has_next_array();
   *
   * @return Whether there is a next element.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> next_element(container c) noexcept;

  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<int64_t> get_int64() noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<double> get_double() noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_really_inline bool is_null() noexcept;

  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<uint64_t> get_root_uint64() noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<int64_t> get_root_int64() noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<double> get_root_double() noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> get_root_bool() noexcept;
  simdjson_really_inline bool root_is_null() noexcept;

  simdjson_really_inline bool in_container(container c) const noexcept;
  simdjson_really_inline container current_container() const noexcept;

  /**
   * If children were left partially iterated / unfinished, this will complete the iteration so we
   * are at a comma or end of document/array/object.
   *
   * @precondition The iterator MUST at or above the given depth.
   * @postcondition The iterator is at the given depth.
   */
  simdjson_really_inline void skip_unfinished_children(container c) noexcept;

  /**
   * Finishes iterating through a container. If values were left partially iterated / unfinished,
   * this will complete the iteration so we are just past the end of the array/object.
   *
   * @precondition The iterator MUST at or above the given depth.
   * @postcondition The iterator is at the given depth.
   */
  simdjson_really_inline void finish(container c) noexcept;

  /**
   * Skips a JSON value, whether it is a scalar, array or object.
   */
  simdjson_really_inline void skip() noexcept;

protected:
  simdjson_really_inline json_iterator(const uint8_t *buf, uint32_t *index, uint32_t depth) noexcept;
  template<int N>
  SIMDJSON_WARN_UNUSED simdjson_really_inline bool advance_to_buffer(uint8_t (&buf)[N]) noexcept;

  uint32_t depth{};

  friend class document;
  friend class object;
  friend class array;
  friend class value;
  friend simdjson_really_inline void logger::log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta) noexcept;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
