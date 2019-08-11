#include "simdjson/simdjson.h"
#include <map>

namespace simdjson {
const std::map<int, const std::string> error_strings = {
    {SUCCESS, "No errors"},
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
    {UNITIALIZED, "Unitialized"},
    {EMPTY, "Empty"},
    {UNESCAPED_CHARS, "Within strings, some characters must be escapted, we "
                      "found unescapted characters"},
    {UNEXPECTED_ERROR, "Unexpected error, consider reporting this problem as "
                       "you may have found a bug in simdjson"},
};

const std::string &error_message(const int error_code) {
  return error_strings.at(error_code);
}
} // namespace simdjson
