#ifndef SIMDJSON_INLINE_ERROR_H
#define SIMDJSON_INLINE_ERROR_H

#include "simdjson/error.h"
#include <string>

namespace simdjson::internal {
  // We store the error code so we can validate the error message is associated with the right code
  struct error_code_info {
    error_code code;
    std::string message;
  };
  // These MUST match the codes in error_code. We check this constraint in basictests.
  inline const error_code_info error_codes[] {
    { SUCCESS, "No error" },
    { SUCCESS_AND_HAS_MORE, "No error and buffer still has more data" },
    { CAPACITY, "This parser can't support a document that big" },
    { MEMALLOC, "Error allocating memory, we're most likely out of memory" },
    { TAPE_ERROR, "Something went wrong while writing to the tape" },
    { DEPTH_ERROR, "The JSON document was too deep (too many nested objects and arrays)" },
    { STRING_ERROR, "Problem while parsing a string" },
    { T_ATOM_ERROR, "Problem while parsing an atom starting with the letter 't'" },
    { F_ATOM_ERROR, "Problem while parsing an atom starting with the letter 'f'" },
    { N_ATOM_ERROR, "Problem while parsing an atom starting with the letter 'n'" },
    { NUMBER_ERROR, "Problem while parsing a number" },
    { UTF8_ERROR, "The input is not valid UTF-8" },
    { UNINITIALIZED, "Uninitialized" },
    { EMPTY, "Empty: no JSON found" },
    { UNESCAPED_CHARS, "Within strings, some characters must be escaped, we found unescaped characters" },
    { UNCLOSED_STRING, "A string is opened, but never closed." },
    { UNSUPPORTED_ARCHITECTURE, "simdjson does not have an implementation supported by this CPU architecture (perhaps it's a non-SIMD CPU?)." },
    { INCORRECT_TYPE, "The JSON element does not have the requested type." },
    { NUMBER_OUT_OF_RANGE, "The JSON number is too large or too small to fit within the requested type." },
    { NO_SUCH_FIELD, "The JSON field referenced does not exist in this object." },
    { IO_ERROR, "Error reading the file." },
    { UNEXPECTED_ERROR, "Unexpected error, consider reporting this problem as you may have found a bug in simdjson" }
  }; // error_messages[]
} // namespace simdjson::internal

namespace simdjson {

inline const char *error_message(error_code error) noexcept {
  // If you're using error_code, we're trusting you got it from the enum.
  return internal::error_codes[int(error)].message.c_str();
}

inline const std::string &error_message(int error) noexcept {
  if (error < 0 || error >= error_code::NUM_ERROR_CODES) {
    return internal::error_codes[UNEXPECTED_ERROR].message;
  }
  return internal::error_codes[error].message;
}

inline std::ostream& operator<<(std::ostream& out, error_code error) noexcept {
  return out << error_message(error);
}

} // namespace simdjson

#endif // SIMDJSON_INLINE_ERROR_H
