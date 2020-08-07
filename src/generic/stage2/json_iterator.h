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

  template<bool STREAMING, typename T>
  WARN_UNUSED really_inline error_code walk_document(T &visitor) noexcept;

  // Start a structural 
  really_inline json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index)
    : buf{_dom_parser.buf},
      next_structural{&_dom_parser.structural_indexes[start_structural_index]},
      dom_parser{_dom_parser} {
  }

  // Get the buffer position of the current structural character
  really_inline char peek_next_char() {
    return buf[*(next_structural)];
  }
  really_inline const uint8_t *advance() {
    return &buf[*(next_structural++)];
  }
  really_inline size_t remaining_len() {
    return dom_parser.len - *(next_structural-1);
  }

  really_inline bool at_end() {
    return next_structural == &dom_parser.structural_indexes[dom_parser.n_structural_indexes];
  }
  really_inline bool at_beginning() {
    return next_structural == dom_parser.structural_indexes.get();
  }

  really_inline void log_value(const char *type) {
    logger::log_line(*this, "", type, "");
  }

  really_inline void log_start_value(const char *type) {
    logger::log_line(*this, "+", type, "");
    if (logger::LOG_ENABLED) { logger::log_depth++; }
  }

  really_inline void log_end_value(const char *type) {
    if (logger::LOG_ENABLED) { logger::log_depth--; }
    logger::log_line(*this, "-", type, "");
  }

  really_inline void log_error(const char *error) {
    logger::log_line(*this, "", "ERROR", error);
  }

private:
  template<typename T>
  WARN_UNUSED really_inline bool empty_object(T &visitor) {
    if (peek_next_char() == '}') {
      advance();
      visitor.empty_object(*this);
      return true;
    }
    return false;
  }
  template<typename T>
  WARN_UNUSED really_inline bool empty_array(T &visitor) {
    if (peek_next_char() == ']') {
      advance();
      visitor.empty_array(*this);
      return true;
    }
    return false;
  }
};

template<bool STREAMING, typename T>
WARN_UNUSED really_inline error_code json_iterator::walk_document(T &visitor) noexcept {
  logger::log_start();

  //
  // Start the document
  //
  if (at_end()) { return EMPTY; }
  SIMDJSON_TRY( visitor.start_document(*this) );

  //
  // Read first value
  //
  {
    auto value = advance();
    switch (*value) {
      case '{': if (!empty_object(visitor)) { goto object_begin; }; break;
      case '[':
        // Make sure the outer array is closed before continuing; otherwise, there are ways we could get
        // into memory corruption. See https://github.com/simdjson/simdjson/issues/906
        if (!STREAMING) {
          if (buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]] != ']') {
            return TAPE_ERROR;
          }
        }
        if (!empty_array(visitor)) { goto array_begin; };
        break;
      default: SIMDJSON_TRY( visitor.root_primitive(*this, value) ); break;
    }
  }
  goto document_end;

//
// Object parser states
//
object_begin:
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  SIMDJSON_TRY( visitor.start_object(*this) );

  {
    auto key = advance();
    if (*key != '"') { log_error("Object does not start with a key"); return TAPE_ERROR; }
    visitor.increment_count(*this);
    SIMDJSON_TRY( visitor.key(*this, key) );
  }

object_field:
  if (unlikely( *advance() != ':' )) { log_error("Missing colon after key in object"); return TAPE_ERROR; }
  {
    auto value = advance();
    switch (*value) {
      case '{': if (!empty_object(visitor)) { goto object_begin; }; break;
      case '[': if (!empty_array(visitor)) { goto array_begin; }; break;
      default: SIMDJSON_TRY( visitor.primitive(*this, value) ); break;
    }
  }

object_continue:
  switch (*advance()) {
    case ',':
      visitor.increment_count(*this);
      {
        auto key = advance();
        if (unlikely( *key != '"' )) { log_error("Key string missing at beginning of field in object"); return TAPE_ERROR; }
        SIMDJSON_TRY( visitor.key(*this, key) );
      }
      goto object_field;
    case '}': SIMDJSON_TRY( visitor.end_object(*this) ); goto scope_end;
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
  SIMDJSON_TRY( visitor.start_array(*this) );
  visitor.increment_count(*this);

array_value:
  {
    auto value = advance();
    switch (*value) {
      case '{': if (!empty_object(visitor)) { goto object_begin; }; break;
      case '[': if (!empty_array(visitor)) { goto array_begin; }; break;
      default: SIMDJSON_TRY( visitor.primitive(*this, value) ); break;
    }
  }

array_continue:
  switch (*advance()) {
    case ',': visitor.increment_count(*this); goto array_value;
    case ']': SIMDJSON_TRY( visitor.end_array(*this) ); goto scope_end;
    default: log_error("Missing comma between array values"); return TAPE_ERROR;
  }

document_end:
  SIMDJSON_TRY( visitor.end_document(*this) );

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
