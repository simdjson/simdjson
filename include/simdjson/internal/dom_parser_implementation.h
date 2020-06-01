#ifndef SIMDJSON_INTERNAL_DOM_PARSER_IMPLEMENTATION_H
#define SIMDJSON_INTERNAL_DOM_PARSER_IMPLEMENTATION_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include <memory>

namespace simdjson {

namespace dom {
class parser;
} // namespace dom

namespace internal {

/**
 * An implementation of simdjson's DOM parser for a particular CPU architecture.
 */
class dom_parser_implementation {
public:
  /**
   * @private For internal implementation use
   *
   * Run a full JSON parse on a single document (stage1 + stage2).
   * 
   * Guaranteed only to be called when capacity > document length.
   *
   * Overridden by each implementation.
   *
   * @param buf The json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len The length of the json document.
   * @param parser The parser object. TODO replace this with dom::document & when state is moved to the implementation.
   * @return The error code, or SUCCESS if there was no error.
   */
  WARN_UNUSED virtual error_code parse(const uint8_t *buf, size_t len, dom::parser &parser) noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Stage 1 of the document parser.
   * 
   * Guaranteed only to be called when capacity > document length.
   *
   * Overridden by each implementation.
   *
   * @param buf The json document to parse.
   * @param len The length of the json document.
   * @param parser The parser object. TODO replace this with structural_indexes & when state is moved to the implementation.
   * @param streaming Whether this is being called by parser::parse_many.
   * @return The error code, or SUCCESS if there was no error.
   */
  WARN_UNUSED virtual error_code stage1(const uint8_t *buf, size_t len, dom::parser &parser, bool streaming) noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Stage 2 of the document parser.
   * 
   * Guaranteed only to be called after stage1(), with the same buf/len as stage1().
   *
   * Overridden by each implementation.
   *
   * @param buf The json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len The length of the json document.
   * @param parser The parser object. TODO replace this with dom::document & when state is moved to the implementation.
   * @return The error code, or SUCCESS if there was no error.
   */
  WARN_UNUSED virtual error_code stage2(const uint8_t *buf, size_t len, dom::parser &parser) noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Stage 2 of the document parser for parser::parse_many.
   *
   * Guaranteed only to be called after stage1(), with buf and len being a subset of the total stage1 buf/len.
   * Overridden by each implementation.
   *
   * @param buf The json document to parse.
   * @param len The length of the json document.
   * @param parser The parser object. TODO replace this with dom::document & when state is moved to the implementation.
   * @param next_json The next structural index. Start this at 0 the first time, and it will be updated to the next value to pass each time.
   * @return The error code, SUCCESS if there was no error, or SUCCESS_AND_HAS_MORE if there was no error and stage2 can be called again.
   */
  WARN_UNUSED virtual error_code stage2(const uint8_t *buf, size_t len, dom::parser &parser, size_t &next_json) noexcept = 0;

  /**
   * Change the capacity of this parser.
   * 
   * Generally used for reallocation.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth.
   * @return The error code, or SUCCESS if there was no error.
   */
  virtual error_code set_capacity(size_t capacity) noexcept = 0;

  /**
   * Change the max depth of this parser.
   *
   * Generally used for reallocation.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth.
   * @return The error code, or SUCCESS if there was no error.
   */
  virtual error_code set_max_depth(size_t max_depth) noexcept = 0;

  virtual ~dom_parser_implementation()=default;

}; // class dom_parser_implementation

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_DOM_PARSER_IMPLEMENTATION_H
