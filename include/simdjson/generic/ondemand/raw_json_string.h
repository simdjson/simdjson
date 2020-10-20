#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class object;
class parser;

/**
 * A string escaped per JSON rules, terminated with quote ("). They are used to represent
 * unescaped keys inside JSON documents.
 *
 * (In other words, a pointer to the beginning of a string, just after the start quote, inside a
 * JSON file.)
 * 
 * This class is deliberately simplistic and has little functionality. You can
 * compare two raw_json_string instances, or compare a raw_json_string with a string_view, but
 * that is pretty much all you can do.
 * 
 * They originate typically from field instance which in turn represent key-value pairs from
 * object instances. From a field instance, you get the raw_json_string instance by calling key().
 * You can, if you want a more usable string_view instance, call the unescaped_key() method
 * on the field instance.
 */
class raw_json_string {
public:
  /**
   * Create a new invalid raw_json_string.
   * 
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline raw_json_string() noexcept = default;

  simdjson_really_inline raw_json_string(const raw_json_string &other) noexcept = default;
  simdjson_really_inline raw_json_string &operator=(const raw_json_string &other) noexcept = default;

  /**
   * Create a new invalid raw_json_string pointed at the given location in the JSON.
   * 
   * The given location must be just *after* the beginning quote (") in the JSON file.
   * 
   * It *must* be terminated by a ", and be a valid JSON string.
   */
  simdjson_really_inline raw_json_string(const uint8_t * _buf) noexcept;
  /**
   * Get the raw pointer to the beginning of the string in the JSON (just after the ").
   * 
   * It is possible for this function to return a null pointer if the instance
   * has outlived its existence.
   */
  simdjson_really_inline const char * raw() const noexcept;

private:
  /**
   * This will set the inner pointer to zero, effectively making
   * this instance unusable.
   */
  simdjson_really_inline void consume() noexcept { buf = nullptr; }

  /**
   * Checks whether the inner pointer is non-null and thus usable.
   */
  simdjson_really_inline simdjson_warn_unused bool alive() const noexcept { return buf != nullptr; }

  /**
   * Unescape this JSON string, replacing \\ with \, \n with newline, etc.
   * 
   * ## IMPORTANT: string_view lifetime
   * 
   * The string_view is only valid as long as the bytes in dst.
   * 
   * @param dst A pointer to a buffer at least large enough to write this string as well as a \0.
   *            dst will be updated to the next unused location (just after the \0 written out at
   *            the end of this string).
   * @return A string_view pointing at the unescaped string in dst
   * @error STRING_ERROR if escapes are incorrect.
   */
  simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> unescape(uint8_t *&dst) const noexcept;
  /**
   * Unescape this JSON string, replacing \\ with \, \n with newline, etc.
   * 
   * ## IMPORTANT: string_view lifetime
   * 
   * The string_view is only valid until the next parse() call on the parser.
   * 
   * @param iter A json_iterator, which contains a buffer where the string will be written.
   */
  simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> unescape(json_iterator &iter) const noexcept;

  const uint8_t * buf{};
  friend class object;
  friend class field;
  friend struct simdjson_result<raw_json_string>;
};

simdjson_unused simdjson_really_inline bool operator==(const raw_json_string &a, std::string_view b) noexcept;
simdjson_unused simdjson_really_inline bool operator==(std::string_view a, const raw_json_string &b) noexcept;
simdjson_unused simdjson_really_inline bool operator!=(const raw_json_string &a, std::string_view b) noexcept;
simdjson_unused simdjson_really_inline bool operator!=(std::string_view a, const raw_json_string &b) noexcept;

simdjson_unused simdjson_really_inline std::ostream &operator<<(std::ostream &, const raw_json_string &) noexcept;

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> &a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private

  simdjson_really_inline simdjson_result<const char *> raw() const noexcept;
  simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> unescape(uint8_t *&dst) const noexcept;
  simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> unescape(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &iter) const noexcept;
};

} // namespace simdjson
