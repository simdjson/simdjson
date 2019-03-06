#ifndef  SIMDJSON_ERR_H
# define SIMDJSON_ERR_H

#include <string>

struct simdjson {
  enum errorValues {
    SUCCESS = 0,
    CAPACITY, // This ParsedJson can't support a document that big
    MEMALLOC, // Error allocating memory, most likely out of memory
    TAPE_ERROR, // Something went wrong while writing to the tape
  };
  static const std::string& errorMsg(const int);
};

#endif