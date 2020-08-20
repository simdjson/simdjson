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
  simdjson_really_inline json_iterator() noexcept;
  simdjson_really_inline json_iterator(json_iterator &&other) noexcept;
  simdjson_really_inline json_iterator &operator=(json_iterator &&other) noexcept;
  simdjson_really_inline json_iterator(const json_iterator &other) noexcept = delete;
  simdjson_really_inline json_iterator &operator=(const json_iterator &other) noexcept = delete;

  /**
   * If children were left partially iterated / unfinished, this will complete the iteration so we
   * are at a comma or end of document/array/object.
   *
   * @precondition The iterator MUST at or above the given depth.
   * @postcondition The iterator is at the given depth.
   */
  simdjson_really_inline void skip_unfinished_children(uint32_t container_depth) noexcept;

  /**
   * Skips a JSON value, whether it is a scalar, array or object.
   */
  simdjson_really_inline void skip_value() noexcept;

protected:
  simdjson_really_inline json_iterator(const uint8_t *buf, uint32_t *index, uint32_t depth) noexcept;

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
