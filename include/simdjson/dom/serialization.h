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

class mini_formatter;

/**
 * The string_builder template allows us to construct
 * a string from a document element. It is parametrized
 * by a "formatter" which handles the details. Thus
 * the string_builder template could support both minification
 * and prettification, and various other tradeoffs.
 */
template <class formatter = mini_formatter> 
class string_builder {
public:
  string_builder() = default;

  inline void append(dom::element value);
  inline void append(dom::array value);
  inline void append(dom::object value);
  inline void append(dom::key_value_pair value);

  inline void clear();
  inline std::string_view str() const;
private:
  formatter format{};
};

/**
 * This is the class that we expect to use with the string_builder
 * template. It tries to produce a compact version of the JSON element
 * as quickly as possible.
 */
class mini_formatter {
public:
  mini_formatter() = default;
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

  inline void clear();
  inline std::string_view str() const;

private:
  // implementation details (subject to change)
  inline void c_str(const char *c);
  inline void one_char(char c);
  std::vector<char> buffer{}; // not ideal!
};



template <class T> 
std::string to_string(T x) {
    simdjson::dom::string_builder<> sb;
    sb.append(x);
    std::string_view answer = sb.str();
    return std::string(answer.data(), answer.size());
}
#if SIMDJSON_EXCEPTIONS
template <class T> 
std::string to_string(simdjson_result<T> x) {
    if (x.error()) { throw simdjson_error(x.error()); }
    return to_string(x.value());
}
#endif 

} // namespace dom
} // namespace simdjson

#endif