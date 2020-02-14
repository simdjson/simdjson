#include "simdjson/error.h"
#include <map>

namespace simdjson {

const std::map<int, const std::string> error_strings = {
    {SUCCESS, "No error"},
    {SUCCESS_AND_HAS_MORE, "No error and buffer still has more data"},
    {CAPACITY, "This ParsedJson can't support a document that big"},
    {MEMALLOC, "Error allocating memory, we're most likely out of memory"},
    {TAPE_ERROR, "Something went wrong while writing to the tape"},
    {STRING_ERROR, "Problem while parsing a string"},
    {T_ATOM_ERROR,
     "Problem while parsing an atom starting with the letter 't'"},
    {F_ATOM_ERROR,
     "Problem while parsing an atom starting with the letter 'f'"},
    {N_ATOM_ERROR,
     "Problem while parsing an atom starting with the letter 'n'"},
    {NUMBER_ERROR, "Problem while parsing a number"},
    {UTF8_ERROR, "The input is not valid UTF-8"},
    {UNINITIALIZED, "Uninitialized"},
    {EMPTY, "Empty: no JSON found"},
    {UNESCAPED_CHARS, "Within strings, some characters must be escaped, we "
                      "found unescaped characters"},
    {UNCLOSED_STRING, "A string is opened, but never closed."},
    {UNEXPECTED_ERROR, "Unexpected error, consider reporting this problem as "
                       "you may have found a bug in simdjson"},
};

// string returned when the error code is not recognized
const std::string unexpected_error_msg {"Unexpected error"};

// returns a string matching the error code
const std::string &error_message(error_code code) {
  auto keyvalue = error_strings.find(code);
  if(keyvalue == error_strings.end()) {
    return unexpected_error_msg;
  }
  return keyvalue->second;
}

} // namespace simdjson
