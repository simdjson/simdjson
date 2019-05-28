#ifndef  SIMDJSON_ERR_H
# define SIMDJSON_ERR_H

#include <string>

struct simdjson {
  enum errorValues {
    SUCCESS = 0,
    CAPACITY, // This ParsedJson can't support a document that big
    MEMALLOC, // Error allocating memory, most likely out of memory
    TAPE_ERROR, // Something went wrong while writing to the tape (stage 2), this is a generic error
    DEPTH_ERROR, // Your document exceeds the user-specified depth limitation
    STRING_ERROR, // Problem while parsing a string
    T_ATOM_ERROR, // Problem while parsing an atom starting with the letter 't'
    F_ATOM_ERROR, // Problem while parsing an atom starting with the letter 'f'
    N_ATOM_ERROR, // Problem while parsing an atom starting with the letter 'n'
    NUMBER_ERROR, // Problem while parsing a number
    UTF8_ERROR // the input is not valid UTF-8
  };
  static const std::string& errorMsg(const int);
};

#endif
