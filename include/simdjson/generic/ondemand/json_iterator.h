#ifndef SIMDJSON_GENERIC_ONDEMAND_JSON_ITERATOR_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_JSON_ITERATOR_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/ondemand/token_iterator.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * Iterates through JSON tokens, keeping track of depth and string buffer.
 *
 * @private This is not intended for external use.
 */
class json_iterator {
protected:
  token_iterator token{};
  ondemand::parser *parser{};
  /**
   * Next free location in the string buffer.
   *
   * Used by raw_json_string::unescape() to have a place to unescape strings to.
   */
  uint8_t *_string_buf_loc{};
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
  error_code error{SUCCESS};
  /**
   * Depth of the current token in the JSON.
   *
   * - 0 = finished with document
   * - 1 = document root value (could be [ or {, not yet known)
   * - 2 = , or } inside root array/object
   * - 3 = key or value inside root array/object.
   */
  depth_t _depth{};
  /**
   * Beginning of the document indexes.
   * Normally we have root == parser->implementation->structural_indexes.get()
   * but this may differ, especially in streaming mode (where we have several
   * documents);
   */
  token_position _root{};
  /**
   * Normally, a json_iterator operates over a single document, but in
   * some cases, we may have a stream of documents. This attribute is meant
   * as meta-data: the json_iterator works the same irrespective of the
   * value of this attribute.
   */
  bool _streaming{false};

public:
  simdjson_inline json_iterator() noexcept = default;
  simdjson_inline json_iterator(json_iterator &&other) noexcept;
  simdjson_inline json_iterator &operator=(json_iterator &&other) noexcept;
  simdjson_inline explicit json_iterator(const json_iterator &other) noexcept = default;
  simdjson_inline json_iterator &operator=(const json_iterator &other) noexcept = default;
  /**
   * Skips a JSON value, whether it is a scalar, array or object.
   */
  simdjson_warn_unused simdjson_inline error_code skip_child(depth_t parent_depth) noexcept;

  /**
   * Tell whether the iterator is still at the start
   */
  simdjson_inline bool at_root() const noexcept;

  /**
   * Tell whether we should be expected to run in streaming
   * mode (iterating over many documents). It is pure metadata
   * that does not affect how the iterator works. It is used by
   * start_root_array() and start_root_object().
   */
  simdjson_inline bool streaming() const noexcept;

  /**
   * Get the root value iterator
   */
  simdjson_inline token_position root_position() const noexcept;
  /**
   * Assert that we are at the document depth (== 1)
   */
  simdjson_inline void assert_at_document_depth() const noexcept;
  /**
   * Assert that we are at the root of the document
   */
  simdjson_inline void assert_at_root() const noexcept;

  /**
   * Tell whether the iterator is at the EOF mark
   */
  simdjson_inline bool at_end() const noexcept;

  /**
   * Tell whether the iterator is live (has not been moved).
   */
  simdjson_inline bool is_alive() const noexcept;

  /**
   * Abandon this iterator, setting depth to 0 (as if the document is finished).
   */
  simdjson_inline void abandon() noexcept;

  /**
   * Advance the current token without modifying depth.
   */
  simdjson_inline const uint8_t *return_current_and_advance() noexcept;

  /**
   * Returns true if there is a single token in the index (i.e., it is
   * a JSON with a scalar value such as a single number).
   *
   * @return whether there is a single token
   */
  simdjson_inline bool is_single_token() const noexcept;

  /**
   * Assert that there are at least the given number of tokens left.
   *
   * Has no effect in release builds.
   */
  simdjson_inline void assert_more_tokens(uint32_t required_tokens=1) const noexcept;
  /**
   * Assert that the given position addresses an actual token (is within bounds).
   *
   * Has no effect in release builds.
   */
  simdjson_inline void assert_valid_position(token_position position) const noexcept;
  /**
   * Get the JSON text for a given token (relative).
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * @param delta The relative position of the token to retrieve. e.g. 0 = next token, -1 = prev token.
   *
   * TODO consider a string_view, assuming the length will get stripped out by the optimizer when
   * it isn't used ...
   */
  simdjson_inline const uint8_t *peek(int32_t delta=0) const noexcept;
  /**
   * Get the maximum length of the JSON text for the current token (or relative).
   *
   * The length will include any whitespace at the end of the token.
   *
   * @param delta The relative position of the token to retrieve. e.g. 0 = next token, -1 = prev token.
   */
  simdjson_inline uint32_t peek_length(int32_t delta=0) const noexcept;
  /**
   * Get a pointer to the current location in the input buffer.
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * You may be pointing outside of the input buffer: it is not generally
   * safe to dereference this pointer.
   */
  simdjson_inline const uint8_t *unsafe_pointer() const noexcept;
  /**
   * Get the JSON text for a given token.
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * @param position The position of the token to retrieve.
   *
   * TODO consider a string_view, assuming the length will get stripped out by the optimizer when
   * it isn't used ...
   */
  simdjson_inline const uint8_t *peek(token_position position) const noexcept;
  /**
   * Get the maximum length of the JSON text for the current token (or relative).
   *
   * The length will include any whitespace at the end of the token.
   *
   * @param position The position of the token to retrieve.
   */
  simdjson_inline uint32_t peek_length(token_position position) const noexcept;
  /**
   * Get the JSON text for the last token in the document.
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * TODO consider a string_view, assuming the length will get stripped out by the optimizer when
   * it isn't used ...
   */
  simdjson_inline const uint8_t *peek_last() const noexcept;

  /**
   * Ascend one level.
   *
   * Validates that the depth - 1 == parent_depth.
   *
   * @param parent_depth the expected parent depth.
   */
  simdjson_inline void ascend_to(depth_t parent_depth) noexcept;

  /**
   * Descend one level.
   *
   * Validates that the new depth == child_depth.
   *
   * @param child_depth the expected child depth.
   */
  simdjson_inline void descend_to(depth_t child_depth) noexcept;
  simdjson_inline void descend_to(depth_t child_depth, int32_t delta) noexcept;

  /**
   * Get current depth.
   */
  simdjson_inline depth_t depth() const noexcept;

  /**
   * Get current (writeable) location in the string buffer.
   */
  simdjson_inline uint8_t *&string_buf_loc() noexcept;

  /**
   * Report an unrecoverable error, preventing further iteration.
   *
   * @param error The error to report. Must not be SUCCESS, UNINITIALIZED, INCORRECT_TYPE, or NO_SUCH_FIELD.
   * @param message An error message to report with the error.
   */
  simdjson_inline error_code report_error(error_code error, const char *message) noexcept;

  /**
   * Log error, but don't stop iteration.
   * @param error The error to report. Must be INCORRECT_TYPE, or NO_SUCH_FIELD.
   * @param message An error message to report with the error.
   */
  simdjson_inline error_code optional_error(error_code error, const char *message) noexcept;

  /**
   * Take an input in json containing max_len characters and attempt to copy it over to tmpbuf, a buffer with
   * N bytes of capacity. It will return false if N is too small (smaller than max_len) of if it is zero.
   * The buffer (tmpbuf) is padded with space characters.
   */
  simdjson_warn_unused simdjson_inline bool copy_to_buffer(const uint8_t *json, uint32_t max_len, uint8_t *tmpbuf, size_t N) noexcept;

  simdjson_inline token_position position() const noexcept;
  /**
   * Write the raw_json_string to the string buffer and return a string_view.
   * Each raw_json_string should be unescaped once, or else the string buffer might
   * overflow.
   */
  simdjson_inline simdjson_result<std::string_view> unescape(raw_json_string in, bool allow_replacement) noexcept;
  simdjson_inline simdjson_result<std::string_view> unescape_wobbly(raw_json_string in) noexcept;

  simdjson_inline void reenter_child(token_position position, depth_t child_depth) noexcept;

  simdjson_inline error_code consume_character(char c) noexcept;
#if SIMDJSON_DEVELOPMENT_CHECKS
  simdjson_inline token_position start_position(depth_t depth) const noexcept;
  simdjson_inline void set_start_position(depth_t depth, token_position position) noexcept;
#endif

  /* Useful for debugging and logging purposes. */
  inline std::string to_string() const noexcept;

  /**
   * Returns the current location in the document if in bounds.
   */
  inline simdjson_result<const char *> current_location() const noexcept;

  /**
   * Updates this json iterator so that it is back at the beginning of the document,
   * as if it had just been created.
   */
  inline void rewind() noexcept;
  /**
   * This checks whether the {,},[,] are balanced so that the document
   * ends with proper zero depth. This requires scanning the whole document
   * and it may be expensive. It is expected that it will be rarely called.
   * It does not attempt to match { with } and [ with ].
   */
  inline bool balanced() const noexcept;
protected:
  simdjson_inline json_iterator(const uint8_t *buf, ondemand::parser *parser) noexcept;
  /// The last token before the end
  simdjson_inline token_position last_position() const noexcept;
  /// The token *at* the end. This points at gibberish and should only be used for comparison.
  simdjson_inline token_position end_position() const noexcept;
  /// The end of the buffer.
  simdjson_inline token_position end() const noexcept;

  friend class document;
  friend class document_stream;
  friend class object;
  friend class array;
  friend class value;
  friend class raw_json_string;
  friend class parser;
  friend class value_iterator;
  template <typename... Args>
  friend simdjson_inline void logger::log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta, logger::log_level level, Args&&... args) noexcept;
  template <typename... Args>
  friend simdjson_inline void logger::log_line(const json_iterator &iter, token_position index, depth_t depth, const char *title_prefix, const char *title, std::string_view detail, logger::log_level level, Args&&... args) noexcept;
}; // json_iterator

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_inline simdjson_result() noexcept = default;
};

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_JSON_ITERATOR_H