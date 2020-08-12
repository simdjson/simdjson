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

  template<bool STREAMING, typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code walk_document(V &visitor) noexcept;

  // Start a structural 
  simdjson_really_inline json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index)
    : buf{_dom_parser.buf},
      next_structural{&_dom_parser.structural_indexes[start_structural_index]},
      dom_parser{_dom_parser} {
  }

  // Get the buffer position of the current structural character
  simdjson_really_inline char peek_next_char() {
    return buf[*(next_structural)];
  }
  simdjson_really_inline const uint8_t *advance() {
    return &buf[*(next_structural++)];
  }
  simdjson_really_inline size_t remaining_len() {
    return dom_parser.len - *(next_structural-1);
  }

  simdjson_really_inline bool at_end() {
    return next_structural == &dom_parser.structural_indexes[dom_parser.n_structural_indexes];
  }
  simdjson_really_inline bool at_beginning() {
    return next_structural == dom_parser.structural_indexes.get();
  }

  simdjson_really_inline void log_value(const char *type) {
    logger::log_line(*this, "", type, "");
  }

  simdjson_really_inline void log_start_value(const char *type) {
    logger::log_line(*this, "+", type, "");
    if (logger::LOG_ENABLED) { logger::log_depth++; }
  }

  simdjson_really_inline void log_end_value(const char *type) {
    if (logger::LOG_ENABLED) { logger::log_depth--; }
    logger::log_line(*this, "-", type, "");
  }

  simdjson_really_inline void log_error(const char *error) {
    logger::log_line(*this, "", "ERROR", error);
  }

  simdjson_really_inline uint8_t last_structural() {
    return buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]];
  }
};

template<bool STREAMING, typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::walk_document(V &visitor) noexcept {
  logger::log_start();

  //
  // Start the document
  //
  if (at_end()) { return EMPTY; }
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
        case '{':
          if (last_structural() != '}') {
            return TAPE_ERROR;
          }
          break;
        case '[':
          if (last_structural() != ']') {
            return TAPE_ERROR;
          }
          break;
      }
    }

    switch (*value) {
      case '{': if (peek_next_char() == '}') { advance(); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (peek_next_char() == ']') { advance(); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_root_primitive(*this, value) ); break;
    }
  }
  goto document_end;

//
// Object parser states
//
object_begin:
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
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
      case '{': if (peek_next_char() == '}') { advance(); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (peek_next_char() == ']') { advance(); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
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
    case '}': SIMDJSON_TRY( visitor.visit_object_end(*this) ); goto scope_end;
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
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  SIMDJSON_TRY( visitor.visit_array_start(*this) );
  SIMDJSON_TRY( visitor.increment_count(*this) );

array_value:
  {
    auto value = advance();
    switch (*value) {
      case '{': if (peek_next_char() == '}') { advance(); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (peek_next_char() == ']') { advance(); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

array_continue:
  switch (*advance()) {
    case ',': SIMDJSON_TRY( visitor.increment_count(*this) ); goto array_value;
    case ']': SIMDJSON_TRY( visitor.visit_array_end(*this) ); goto scope_end;
    default: log_error("Missing comma between array values"); return TAPE_ERROR;
  }

document_end:
  SIMDJSON_TRY( visitor.visit_document_end(*this) );

  dom_parser.next_structural_index = uint32_t(next_structural - &dom_parser.structural_indexes[0]);

  // If we didn't make it to the end, it's an error
  if ( !STREAMING && dom_parser.next_structural_index != dom_parser.n_structural_indexes ) {
    log_error("More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
    return TAPE_ERROR;
  }

  return SUCCESS;

} // walk_document()

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
