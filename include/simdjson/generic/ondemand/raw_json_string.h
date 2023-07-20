#ifndef SIMDJSON_GENERIC_ONDEMAND_RAW_JSON_STRING_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_RAW_JSON_STRING_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * A string escaped per JSON rules, terminated with quote ("). They are used to represent
 * unescaped keys inside JSON documents.
 *
 * (In other words, a pointer to the beginning of a string, just after the start quote, inside a
 * JSON file.)
 *
 * This class is deliberately simplistic and has little functionality. You can
 * compare a raw_json_string instance with an unescaped C string, but
 * that is nearly all you can do.
 *
 * The raw_json_string is unescaped. If you wish to write an unescaped version of it to your own
 * buffer, you may do so using the parser.unescape(string, buff) method, using an ondemand::parser
 * instance. Doing so requires you to have a sufficiently large buffer.
 *
 * The raw_json_string instances originate typically from field instance which in turn represent
 * key-value pairs from object instances. From a field instance, you get the raw_json_string
 * instance by calling key(). You can, if you want a more usable string_view instance, call
 * the unescaped_key() method on the field instance. You may also create a raw_json_string from
 * any other string value, with the value.get_raw_json_string() method. Again, you can get
 * a more usable string_view instance by calling get_string().
 *
 */
class raw_json_string {
public:
  /**
   * Create a new invalid raw_json_string.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_inline raw_json_string() noexcept = default;

  /**
   * Create a new invalid raw_json_string pointed at the given location in the JSON.
   *
   * The given location must be just *after* the beginning quote (") in the JSON file.
   *
   * It *must* be terminated by a ", and be a valid JSON string.
   */
  simdjson_inline raw_json_string(const uint8_t * _buf) noexcept;
  /**
   * Get the raw pointer to the beginning of the string in the JSON (just after the ").
   *
   * It is possible for this function to return a null pointer if the instance
   * has outlived its existence.
   */
  simdjson_inline const char * raw() const noexcept;

  /**
   * This compares the current instance to the std::string_view target: returns true if
   * they are byte-by-byte equal (no escaping is done) on target.size() characters,
   * and if the raw_json_string instance has a quote character at byte index target.size().
   * We never read more than length + 1 bytes in the raw_json_string instance.
   * If length is smaller than target.size(), this will return false.
   *
   * The std::string_view instance may contain any characters. However, the caller
   * is responsible for setting length so that length bytes may be read in the
   * raw_json_string.
   *
   * Performance: the comparison may be done using memcmp which may be efficient
   * for long strings.
   */
  simdjson_inline bool unsafe_is_equal(size_t length, std::string_view target) const noexcept;

  /**
   * This compares the current instance to the std::string_view target: returns true if
   * they are byte-by-byte equal (no escaping is done).
   * The std::string_view instance should not contain unescaped quote characters:
   * the caller is responsible for this check. See is_free_from_unescaped_quote.
   *
   * Performance: the comparison is done byte-by-byte which might be inefficient for
   * long strings.
   *
   * If target is a compile-time constant, and your compiler likes you,
   * you should be able to do the following without performance penalty...
   *
   *   static_assert(raw_json_string::is_free_from_unescaped_quote(target), "");
   *   s.unsafe_is_equal(target);
   */
  simdjson_inline bool unsafe_is_equal(std::string_view target) const noexcept;

  /**
   * This compares the current instance to the C string target: returns true if
   * they are byte-by-byte equal (no escaping is done).
   * The provided C string should not contain an unescaped quote character:
   * the caller is responsible for this check. See is_free_from_unescaped_quote.
   *
   * If target is a compile-time constant, and your compiler likes you,
   * you should be able to do the following without performance penalty...
   *
   *   static_assert(raw_json_string::is_free_from_unescaped_quote(target), "");
   *   s.unsafe_is_equal(target);
   */
  simdjson_inline bool unsafe_is_equal(const char* target) const noexcept;

  /**
   * This compares the current instance to the std::string_view target: returns true if
   * they are byte-by-byte equal (no escaping is done).
   */
  simdjson_inline bool is_equal(std::string_view target) const noexcept;

  /**
   * This compares the current instance to the C string target: returns true if
   * they are byte-by-byte equal (no escaping is done).
   */
  simdjson_inline bool is_equal(const char* target) const noexcept;

  /**
   * Returns true if target is free from unescaped quote. If target is known at
   * compile-time, we might expect the computation to happen at compile time with
   * many compilers (not all!).
   */
  static simdjson_inline bool is_free_from_unescaped_quote(std::string_view target) noexcept;
  static simdjson_inline bool is_free_from_unescaped_quote(const char* target) noexcept;

private:


  /**
   * This will set the inner pointer to zero, effectively making
   * this instance unusable.
   */
  simdjson_inline void consume() noexcept { buf = nullptr; }

  /**
   * Checks whether the inner pointer is non-null and thus usable.
   */
  simdjson_inline simdjson_warn_unused bool alive() const noexcept { return buf != nullptr; }

  /**
   * Unescape this JSON string, replacing \\ with \, \n with newline, etc.
   * The result will be a valid UTF-8.
   *
   * ## IMPORTANT: string_view lifetime
   *
   * The string_view is only valid until the next parse() call on the parser.
   *
   * @param iter A json_iterator, which contains a buffer where the string will be written.
   * @param allow_replacement Whether we allow replacement of invalid surrogate pairs.
   */
  simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> unescape(json_iterator &iter, bool allow_replacement) const noexcept;

  /**
   * Unescape this JSON string, replacing \\ with \, \n with newline, etc.
   * The result may not be a valid UTF-8. https://simonsapin.github.io/wtf-8/
   *
   * ## IMPORTANT: string_view lifetime
   *
   * The string_view is only valid until the next parse() call on the parser.
   *
   * @param iter A json_iterator, which contains a buffer where the string will be written.
   */
  simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> unescape_wobbly(json_iterator &iter) const noexcept;
  const uint8_t * buf{};
  friend class object;
  friend class field;
  friend class parser;
  friend struct simdjson_result<raw_json_string>;
};

simdjson_unused simdjson_inline std::ostream &operator<<(std::ostream &, const raw_json_string &) noexcept;

/**
 * Comparisons between raw_json_string and std::string_view instances are potentially unsafe: the user is responsible
 * for providing a string with no unescaped quote. Note that unescaped quotes cannot be present in valid JSON strings.
 */
simdjson_unused simdjson_inline bool operator==(const raw_json_string &a, std::string_view c) noexcept;
simdjson_unused simdjson_inline bool operator==(std::string_view c, const raw_json_string &a) noexcept;
simdjson_unused simdjson_inline bool operator!=(const raw_json_string &a, std::string_view c) noexcept;
simdjson_unused simdjson_inline bool operator!=(std::string_view c, const raw_json_string &a) noexcept;


} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_inline simdjson_result() noexcept = default;
  simdjson_inline ~simdjson_result() noexcept = default; ///< @private

  simdjson_inline simdjson_result<const char *> raw() const noexcept;
  simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> unescape(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &iter, bool allow_replacement) const noexcept;
  simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> unescape_wobbly(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &iter) const noexcept;
};

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_RAW_JSON_STRING_H