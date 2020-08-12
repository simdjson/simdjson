#include "generic/stage2/logger.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

class json_iterator {
public:
  const uint8_t* const buf;
  uint32_t *next_structural;
  dom_parser_implementation &dom_parser;
  uint32_t depth{0};

  /**
   * Walk the JSON document.
   *
   * The visitor receives callbacks when values are encountered. All callbacks pass the iterator as
   * the first parameter; some callbacks have other parameters as well:
   *
   * - visit_document_start() - at the beginning.
   * - visit_document_end() - at the end (if things were successful).
   *
   * - visit_array_start() - at the start `[` of a non-empty array.
   * - visit_array_end() - at the end `]` of a non-empty array.
   * - visit_empty_array() - when an empty array is encountered.
   *
   * - visit_object_end() - at the start `]` of a non-empty object.
   * - visit_object_start() - at the end `]` of a non-empty object.
   * - visit_empty_object() - when an empty object is encountered.
   * - visit_key(const uint8_t *key) - when a key in an object field is encountered. key is
   *                                   guaranteed to point at the first quote of the string (`"key"`).
   * - visit_primitive(const uint8_t *value) - when a value is a string, number, boolean or null.
   * - visit_root_primitive(iter, uint8_t *value) - when the top-level value is a string, number, boolean or null.
   *
   * - increment_count(iter) - each time a value is found in an array or object.
   */
  template<bool STREAMING, typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code walk_document(V &visitor) noexcept;

  /**
   * Create an iterator capable of walking a JSON document.
   *
   * The document must have already passed through stage 1.
   */
  simdjson_really_inline json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index);

  /**
   * Look at the next token.
   *
   * Tokens can be strings, numbers, booleans, null, or operators (`[{]},:`)).
   *
   * They may include invalid JSON as well (such as `1.2.3` or `ture`).
   */
  simdjson_really_inline const uint8_t *peek() const noexcept;
  /**
   * Advance to the next token.
   *
   * Tokens can be strings, numbers, booleans, null, or operators (`[{]},:`)).
   *
   * They may include invalid JSON as well (such as `1.2.3` or `ture`).
   */
  simdjson_really_inline const uint8_t *advance() noexcept;
  /**
   * Get the remaining length of the document, from the start of the current token.
   */
  simdjson_really_inline size_t remaining_len() const noexcept;
  /**
   * Check if we are at the end of the document.
   *
   * If this is true, there are no more tokens.
   */
  simdjson_really_inline bool at_eof() const noexcept;
  /**
   * Check if we are at the beginning of the document.
   */
  simdjson_really_inline bool at_beginning() const noexcept;
  simdjson_really_inline uint8_t last_structural() const noexcept;

  /**
   * Log that a value has been found.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_value(const char *type) const noexcept;
  /**
   * Log the start of a multipart value.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_start_value(const char *type) const noexcept;
  /**
   * Log the end of a multipart value.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_end_value(const char *type) const noexcept;
  /**
   * Log an error.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_error(const char *error) const noexcept;

  template<typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_primitive(V &visitor, const uint8_t *value) noexcept;
  template<typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_primitive(V &visitor, const uint8_t *value) noexcept;
};

template<bool STREAMING, typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::walk_document(V &visitor) noexcept {
  logger::log_start();

  //
  // Start the document
  //
  if (at_eof()) { return EMPTY; }
  log_start_value("document");
  SIMDJSON_TRY( visitor.visit_document_start(*this) );

  //
  // Read first value
  //
  {
    auto value = advance();

    // Make sure the outer hash or array is closed before continuing; otherwise, there are ways we
    // could get into memory corruption. See https://github.com/simdjson/simdjson/issues/906
    if (!STREAMING) {
      switch (*value) {
        case '{': if (last_structural() != '}') { return TAPE_ERROR; }; break;
        case '[': if (last_structural() != ']') { return TAPE_ERROR; }; break;
      }
    }

    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_root_primitive(*this, value) ); break;
    }
  }
  goto document_end;

//
// Object parser states
//
object_begin:
  log_start_value("object");
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  dom_parser.is_array[depth] = false;
  SIMDJSON_TRY( visitor.visit_object_start(*this) );

  {
    auto key = advance();
    if (*key != '"') { log_error("Object does not start with a key"); return TAPE_ERROR; }
    SIMDJSON_TRY( visitor.increment_count(*this) );
    SIMDJSON_TRY( visitor.visit_key(*this, key) );
  }

object_field:
  if (simdjson_unlikely( *advance() != ':' )) { log_error("Missing colon after key in object"); return TAPE_ERROR; }
  {
    auto value = advance();
    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

object_continue:
  switch (*advance()) {
    case ',':
      SIMDJSON_TRY( visitor.increment_count(*this) );
      {
        auto key = advance();
        if (simdjson_unlikely( *key != '"' )) { log_error("Key string missing at beginning of field in object"); return TAPE_ERROR; }
        SIMDJSON_TRY( visitor.visit_key(*this, key) );
      }
      goto object_field;
    case '}': log_end_value("object"); SIMDJSON_TRY( visitor.visit_object_end(*this) ); goto scope_end;
    default: log_error("No comma between object fields"); return TAPE_ERROR;
  }

scope_end:
  depth--;
  if (depth == 0) { goto document_end; }
  if (dom_parser.is_array[depth]) { goto array_continue; }
  goto object_continue;

//
// Array parser states
//
array_begin:
  log_start_value("array");
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  dom_parser.is_array[depth] = true;
  SIMDJSON_TRY( visitor.visit_array_start(*this) );
  SIMDJSON_TRY( visitor.increment_count(*this) );

array_value:
  {
    auto value = advance();
    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

array_continue:
  switch (*advance()) {
    case ',': SIMDJSON_TRY( visitor.increment_count(*this) ); goto array_value;
    case ']': log_end_value("array"); SIMDJSON_TRY( visitor.visit_array_end(*this) ); goto scope_end;
    default: log_error("Missing comma between array values"); return TAPE_ERROR;
  }

document_end:
  log_end_value("document");
  SIMDJSON_TRY( visitor.visit_document_end(*this) );

  dom_parser.next_structural_index = uint32_t(next_structural - &dom_parser.structural_indexes[0]);

  // If we didn't make it to the end, it's an error
  if ( !STREAMING && dom_parser.next_structural_index != dom_parser.n_structural_indexes ) {
    log_error("More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
    return TAPE_ERROR;
  }

  return SUCCESS;

} // walk_document()

simdjson_really_inline json_iterator::json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index)
  : buf{_dom_parser.buf},
    next_structural{&_dom_parser.structural_indexes[start_structural_index]},
    dom_parser{_dom_parser} {
}

simdjson_really_inline const uint8_t *json_iterator::peek() const noexcept {
  return &buf[*(next_structural)];
}
simdjson_really_inline const uint8_t *json_iterator::advance() noexcept {
  return &buf[*(next_structural++)];
}
simdjson_really_inline size_t json_iterator::remaining_len() const noexcept {
  return dom_parser.len - *(next_structural-1);
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return next_structural == &dom_parser.structural_indexes[dom_parser.n_structural_indexes];
}
simdjson_really_inline bool json_iterator::at_beginning() const noexcept {
  return next_structural == dom_parser.structural_indexes.get();
}
simdjson_really_inline uint8_t json_iterator::last_structural() const noexcept {
  return buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]];
}

simdjson_really_inline void json_iterator::log_value(const char *type) const noexcept {
  logger::log_line(*this, "", type, "");
}

simdjson_really_inline void json_iterator::log_start_value(const char *type) const noexcept {
  logger::log_line(*this, "+", type, "");
  if (logger::LOG_ENABLED) { logger::log_depth++; }
}

simdjson_really_inline void json_iterator::log_end_value(const char *type) const noexcept {
  if (logger::LOG_ENABLED) { logger::log_depth--; }
  logger::log_line(*this, "-", type, "");
}

simdjson_really_inline void json_iterator::log_error(const char *error) const noexcept {
  logger::log_line(*this, "", "ERROR", error);
}

template<typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::visit_root_primitive(V &visitor, const uint8_t *value) noexcept {
  switch (*value) {
    case '"': return visitor.visit_root_string(*this, value);
    case 't': return visitor.visit_root_true_atom(*this, value);
    case 'f': return visitor.visit_root_false_atom(*this, value);
    case 'n': return visitor.visit_root_null_atom(*this, value);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return visitor.visit_root_number(*this, value);
    default:
      log_error("Document starts with a non-value character");
      return TAPE_ERROR;
  }
}
template<typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::visit_primitive(V &visitor, const uint8_t *value) noexcept {
  switch (*value) {
    case '"': return visitor.visit_string(*this, value);
    case 't': return visitor.visit_true_atom(*this, value);
    case 'f': return visitor.visit_false_atom(*this, value);
    case 'n': return visitor.visit_null_atom(*this, value);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return visitor.visit_number(*this, value);
    default:
      log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
