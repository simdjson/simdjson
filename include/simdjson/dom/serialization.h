#ifndef SIMDJSON_SERIALIZATION_H
#define SIMDJSON_SERIALIZATION_H

#include "simdjson/common_defs.h"
#include "simdjson/dom/document.h"
#include "simdjson/error.h"
#include "simdjson/internal/dom_parser_implementation.h"
#include "simdjson/internal/tape_ref.h"
#include "simdjson/minify.h"
#include "simdjson/padded_string.h"
#include "simdjson/portability.h"
#include <vector>

namespace simdjson {
namespace dom {


template <class formatter> struct string_buffer {
  string_buffer() : format(buffer) {}

  inline void append(dom::element value);
  inline void append(dom::array value);
  inline void append(dom::object value);
  inline void append(dom::key_value_pair value);

  formatter format;
  std::vector<char> buffer;
};


class mini_formatter {
public:
  mini_formatter(std::vector<char> &b) : buffer(b) {}
  inline void comma();
  inline void start_array();
  inline void end_array();
  inline void start_object();
  inline void end_object();
  inline void true_atom();
  inline void false_atom();
  inline void null_atom();
  inline void number(int64_t x);
  inline void number(uint64_t x);  
  inline void number(double x);
  inline void key(std::string_view unescaped);
  inline void string(std::string_view unescaped);
private:
  // implementation details (subject to change)
  inline void c_str(const char *c);
  inline void one_char(char c);  
  std::vector<char>& buffer; // not ideal!
};



} // dom
} // namespace simdjson 

#endif