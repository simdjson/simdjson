// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2.h" (this simplifies amalgation)

#include "generic/stage2/logger.h"
#include "generic/stage2/structural_iterator.h"

namespace { // Make everything here private
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct structural_parser : structural_iterator {
  /** Current depth (nested objects and arrays) */
  uint32_t depth{0};

  template<bool STREAMING, typename T>
  WARN_UNUSED really_inline error_code parse(T &builder) noexcept;

  // For non-streaming, to pass an explicit 0 as next_structural, which enables optimizations
  really_inline structural_parser(dom_parser_implementation &_dom_parser)
    : structural_iterator(_dom_parser) {
  }
}; // struct structural_parser

template<bool STREAMING, typename T>
WARN_UNUSED really_inline error_code structural_parser::parse(T &builder) noexcept {
  logger::log_start();

  //
  // Start the document
  //
  if (at_end()) { return builder.error(EMPTY, "Empty document"); }

  SIMDJSON_TRY( builder.start_document() );

  const uint8_t *value;

  //
  // Read first value
  //
  {
    switch (*(value = advance())) {
      case '{':
        goto generic_object_begin;
      case '[': {
        // Make sure the outer array is closed before continuing; otherwise, there are ways we could get
        // into memory corruption. See https://github.com/simdjson/simdjson/issues/906
        if (!STREAMING) {
          if (buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]] != ']') {
            return TAPE_ERROR;
          }
        }
        goto generic_array_begin;
      }
      default: SIMDJSON_TRY( builder.root_primitive(value) );
    }
    goto document_end;
  }

//
// Object parser states
//
generic_object_begin: {
  switch (*(value = advance())) {
    case '}': SIMDJSON_TRY( builder.empty_object() ); goto generic_next;
    case '"': SIMDJSON_TRY( builder.start_object() ); goto object_colon;
    default: return builder.error(TAPE_ERROR, "First field of object missing key");
  }
} // generic_object_begin:

object_colon: {
  if (advance_char() != ':') { return builder.error(TAPE_ERROR, "First field of object missing :"); }
} // object_colon:

object_value: {
  const uint8_t *key = value;
  switch (*(value = advance())) {
    case '{':
      switch (*(value = advance())) {
        case '}': SIMDJSON_TRY( builder.empty_object_field(key) ); goto object_next;
        case '"': SIMDJSON_TRY( builder.start_object_field(key) ); key = value; goto object_colon;
        default:  return builder.error(TAPE_ERROR, "First field of object missing key");
      }
    case '[':
      switch (*(value = advance())) {
        case ']': SIMDJSON_TRY( builder.empty_array_field(key) ); goto object_next;
        default: SIMDJSON_TRY( builder.start_array_field(key) ); goto array_value;
      }
    default:
      SIMDJSON_TRY( builder.primitive_field(key, value) ); goto object_next;
  }
} // object_value:

object_next: {
  switch (advance_char()) {
    case ',':
      value = advance();
      if (*value != '"') { return builder.error(TAPE_ERROR, "No key in object field"); }
      goto object_colon;
    case '}': SIMDJSON_TRY( builder.end_object() ); goto generic_next;
    default: return builder.error(TAPE_ERROR, "No comma between object fields");
  }
} // object_next:

generic_array_begin: {
  switch (*(value = advance())) {
    case ']': SIMDJSON_TRY( builder.empty_array() ); goto generic_next;
    default: SIMDJSON_TRY( builder.start_array() ); goto array_value;
  }
} // generic_array_begin:

array_value: {
  // TODO this is sort of a hiccup; it forces array_next to advance() to match
  // the fact that generic_array_begin already advances. See if refactoring that helps or not.
  switch (*value) {
    case '{':
      switch (*(value = advance())) {
        case '}': SIMDJSON_TRY( builder.empty_object() ); goto array_next;
        case '"': SIMDJSON_TRY( builder.start_object() ); goto object_colon;
        default:  return builder.error(TAPE_ERROR, "First field of object missing key");
      }
    case '[':
      switch (*(value = advance())) {
        case ']': SIMDJSON_TRY( builder.empty_array() ); goto array_next;
        default: SIMDJSON_TRY( builder.start_array() ); goto array_value;
      }
    default:
      SIMDJSON_TRY( builder.primitive(value) ); goto array_next;
  }
} // array_value:

array_next: {
  switch (advance_char()) {
    case ',':
      value = advance();
      goto array_value;
    case ']':
      SIMDJSON_TRY( builder.end_array() ); goto generic_next;
    default:
      return builder.error(TAPE_ERROR, "Missing comma between fields");
  }
} // array_next:

//
// After a value, when we don't know yet what we're going to see ...
//
// , "key": - object
// , "key", - array
// , "key"] - array
// , <value> - array
// ]
// }
//
generic_next: {
  switch (advance_char()) {
  case ',':
    // The next thing after the comma is either a key or a value.
    switch (*(value = advance())) {
      case '"':
        switch (advance_char()) {
          // "key": ... -> object
          // "value", ... -> array with string value
          // "value"] -> end of array with string value
          case ':': SIMDJSON_TRY( builder.try_resume_object() ); goto object_value;
          case ',': SIMDJSON_TRY( builder.try_resume_array(value) ); goto array_value;
          case ']': SIMDJSON_TRY( builder.try_resume_array(value) ); SIMDJSON_TRY( builder.end_array() ); goto generic_next;
          default: return builder.error(TAPE_ERROR, "Missing comma or colon between values");
        }
      // , [ ... -> array with array value
      // , { ... -> array with object value
      // , <value> -> array with primitive value
      case '[': SIMDJSON_TRY( builder.try_resume_array() ); goto generic_array_begin;
      case '{': SIMDJSON_TRY( builder.try_resume_array() ); goto generic_object_begin;
      default: SIMDJSON_TRY( builder.try_resume_array() ); goto array_value;
    }
  // ] -> end array, still unsure what comes next
  // } -> end object, still unsure what comes next
  case ']': SIMDJSON_TRY( builder.try_end_array() ); goto generic_next; // TODO while loop?
  case '}': SIMDJSON_TRY( builder.try_end_object() ); goto generic_next; // TODO while loop?
  default:
    // If we just ended an array or object, and don't get ] } or , then we might be at document end.
    // We are guaranteed ] } and , will never be at document end.
    // Since we overcorrected, we back up one element.
    next_structural--;
    goto document_end;
  }
} // generic_next:

document_end: {
  SIMDJSON_TRY( builder.end_document() );

  dom_parser.next_structural_index = uint32_t(next_structural - &dom_parser.structural_indexes[0]);

  if (depth != 0) { return builder.error(TAPE_ERROR, "Unclosed objects or arrays!"); }

  // If we didn't make it to the end, it's an error
  if ( !STREAMING && dom_parser.next_structural_index != dom_parser.n_structural_indexes ) {
    return builder.error(TAPE_ERROR, "More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
  }

  return SUCCESS;
} // document_end:

} // parse_structurals()

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
