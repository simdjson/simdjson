#include "simdjson/base.h"

namespace simdjson {
namespace internal {
  /**
   * We include in the error messages the name of the enum
   * as a quality-of-life feature for programmers who receive
   * the error. If they get a text error, they sometimes want
   * to trace back the error code, and it is handy if it is
   * right there.
   */

  SIMDJSON_DLLIMPORTEXPORT const error_code_info error_codes[] {
    { SUCCESS, "No error (SUCCESS)" },
    { CAPACITY, "This parser can't support a document that big (CAPACITY)" },
    { MEMALLOC, "Error allocating memory, we're most likely out of memory (MEMALLOC)" },
    { TAPE_ERROR, "The JSON document has an improper structure: missing or superfluous commas, braces, missing keys, etc. (TAPE_ERROR)" },
    { DEPTH_ERROR, "The JSON document was too deep (too many nested objects and arrays) (DEPTH_ERROR)" },
    { STRING_ERROR, "Problem while parsing a string (STRING_ERROR)" },
    { T_ATOM_ERROR, "Problem while parsing an atom starting with the letter 't' (T_ATOM_ERROR)" },
    { F_ATOM_ERROR, "Problem while parsing an atom starting with the letter 'f' (F_ATOM_ERROR)" },
    { N_ATOM_ERROR, "Problem while parsing an atom starting with the letter 'n' (N_ATOM_ERROR)" },
    { NUMBER_ERROR, "Problem while parsing a number (NUMBER_ERROR)" },
    { UTF8_ERROR, "The input is not valid UTF-8 (UTF8_ERROR)" },
    { UNINITIALIZED, "Uninitialized (UNINITIALIZED)" },
    { EMPTY, "Empty: no JSON found (EMPTY)" },
    { UNESCAPED_CHARS, "Within strings, some characters must be escaped, we found unescaped characters (UNESCAPED_CHARS)" },
    { UNCLOSED_STRING, "A string is opened, but never closed (UNCLOSED_STRING)" },
    { UNSUPPORTED_ARCHITECTURE, "simdjson does not have an implementation supported by this CPU architecture (UNSUPPORTED_ARCHITECTURE)" },
    { INCORRECT_TYPE, "The JSON element does not have the requested type (INCORRECT_TYPE)" },
    { NUMBER_OUT_OF_RANGE, "The JSON number is too large or too small to fit within the requested type (NUMBER_OUT_OF_RANGE)" },
    { INDEX_OUT_OF_BOUNDS, "Attempted to access an element of a JSON array that is beyond its length (INDEX_OUT_OF_BOUNDS)" },
    { NO_SUCH_FIELD, "The JSON field referenced does not exist in this object (NO_SUCH_FIELD)" },
    { IO_ERROR, "Error reading the file (IO_ERROR)" },
    { INVALID_JSON_POINTER, "Invalid JSON pointer syntax (INVALID_JSON_POINTER)" },
    { INVALID_URI_FRAGMENT, "Invalid URI fragment syntax (INVALID_URI_FRAGMENT)" },
    { UNEXPECTED_ERROR, "Unexpected error, consider reporting this problem as you may have found a bug in simdjson (UNEXPECTED_ERROR)" },
    { PARSER_IN_USE, "Cannot parse a new document while a document is still in use (PARSER_IN_USE)" },
    { OUT_OF_ORDER_ITERATION, "Objects and arrays can only be iterated when they are first encountered (OUT_OF_ORDER_ITERATION)" },
    { INSUFFICIENT_PADDING, "simdjson requires the input JSON string to have at least SIMDJSON_PADDING extra bytes allocated, beyond the string's length (INSUFFICIENT_PADDING)" },
    { INCOMPLETE_ARRAY_OR_OBJECT, "JSON document ended early in the middle of an object or array (INCOMPLETE_ARRAY_OR_OBJECT)" }
  }; // error_messages[]

} // namespace internal
} // namespace simdjson