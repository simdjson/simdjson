#ifndef SIMDJSON_SRC_ERROR_TABLES_CPP
#define SIMDJSON_SRC_ERROR_TABLES_CPP

#include <simdjson/internal/jsoncharutils_tables.h>
#include <simdjson/error-inl.h>

namespace simdjson {
namespace internal {

  SIMDJSON_DLLIMPORTEXPORT const error_code_info error_codes[] {
    { SUCCESS, "SUCCESS: No error" },
    { CAPACITY, "CAPACITY: This parser can't support a document that big" },
    { MEMALLOC, "MEMALLOC: Error allocating memory, we're most likely out of memory" },
    { TAPE_ERROR, "TAPE_ERROR: The JSON document has an improper structure: missing or superfluous commas, braces, missing keys, etc.  This is a fatal and unrecoverable error." },
    { DEPTH_ERROR, "DEPTH_ERROR: The JSON document was too deep (too many nested objects and arrays)" },
    { STRING_ERROR, "STRING_ERROR: Problem while parsing a string" },
    { T_ATOM_ERROR, "T_ATOM_ERROR: Problem while parsing an atom starting with the letter 't'" },
    { F_ATOM_ERROR, "F_ATOM_ERROR: Problem while parsing an atom starting with the letter 'f'" },
    { N_ATOM_ERROR, "N_ATOM_ERROR: Problem while parsing an atom starting with the letter 'n'" },
    { NUMBER_ERROR, "NUMBER_ERROR: Problem while parsing a number" },
    { BIGINT_ERROR, "BIGINT_ERROR: Big integer value that cannot be represented using 64 bits" },
    { UTF8_ERROR, "UTF8_ERROR: The input is not valid UTF-8" },
    { UNINITIALIZED, "UNINITIALIZED: Uninitialized" },
    { EMPTY, "EMPTY: no JSON found" },
    { UNESCAPED_CHARS, "UNESCAPED_CHARS: Within strings, some characters must be escaped, we found unescaped characters" },
    { UNCLOSED_STRING, "UNCLOSED_STRING: A string is opened, but never closed." },
    { UNSUPPORTED_ARCHITECTURE, "UNSUPPORTED_ARCHITECTURE: simdjson does not have an implementation supported by this CPU architecture. Please report this error to the core team as it should never happen." },
    { INCORRECT_TYPE, "INCORRECT_TYPE: The JSON element does not have the requested type." },
    { NUMBER_OUT_OF_RANGE, "NUMBER_OUT_OF_RANGE: The JSON number is too large or too small to fit within the requested type." },
    { INDEX_OUT_OF_BOUNDS, "INDEX_OUT_OF_BOUNDS: Attempted to access an element of a JSON array that is beyond its length." },
    { NO_SUCH_FIELD, "NO_SUCH_FIELD: The JSON field referenced does not exist in this object." },
    { IO_ERROR, "IO_ERROR: Error reading the file." },
    { INVALID_JSON_POINTER, "INVALID_JSON_POINTER: Invalid JSON pointer syntax." },
    { INVALID_URI_FRAGMENT, "INVALID_URI_FRAGMENT: Invalid URI fragment syntax." },
    { UNEXPECTED_ERROR, "UNEXPECTED_ERROR: Unexpected error, consider reporting this problem as you may have found a bug in simdjson" },
    { PARSER_IN_USE, "PARSER_IN_USE: Cannot parse a new document while a document is still in use." },
    { OUT_OF_ORDER_ITERATION, "OUT_OF_ORDER_ITERATION: Objects and arrays can only be iterated when they are first encountered." },
    { INSUFFICIENT_PADDING, "INSUFFICIENT_PADDING: simdjson requires the input JSON string to have at least SIMDJSON_PADDING extra bytes allocated, beyond the string's length. Consider using the simdjson::padded_string class if needed." },
    { INCOMPLETE_ARRAY_OR_OBJECT, "INCOMPLETE_ARRAY_OR_OBJECT: JSON document ended early in the middle of an object or array. This is a fatal and unrecoverable error." },
    { SCALAR_DOCUMENT_AS_VALUE, "SCALAR_DOCUMENT_AS_VALUE: A JSON document made of a scalar (number, Boolean, null or string) is treated as a value. Use get_bool(), get_double(), etc. on the document instead. "},
    { OUT_OF_BOUNDS, "OUT_OF_BOUNDS: Attempt to access location outside of document."},
    { TRAILING_CONTENT, "TRAILING_CONTENT: Unexpected trailing content in the JSON input."}
  }; // error_messages[]

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_SRC_ERROR_TABLES_CPP