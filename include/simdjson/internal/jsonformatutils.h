#ifndef SIMDJSON_INTERNAL_JSONFORMATUTILS_H
#define SIMDJSON_INTERNAL_JSONFORMATUTILS_H

#include "simdjson/base.h"
#include <iomanip>
#include <ostream>
#include <sstream>

namespace simdjson {
namespace internal {

inline std::ostream& operator<<(std::ostream& out, const escape_json_string &str);

class escape_json_string {
public:
  escape_json_string(std::string_view _str) noexcept : str{_str} {}
  operator std::string() const noexcept { std::stringstream s; s << *this; return s.str(); }
private:
  std::string_view str;
  friend std::ostream& operator<<(std::ostream& out, const escape_json_string &unescaped);
};

inline std::ostream& operator<<(std::ostream& out, const escape_json_string &unescaped) {
  for (size_t i=0; i<unescaped.str.length(); i++) {
    switch (unescaped.str[i]) {
    case '\b':
      out << "\\b";
      break;
    case '\f':
      out << "\\f";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\"':
      out << "\\\"";
      break;
    case '\t':
      out << "\\t";
      break;
    case '\\':
      out << "\\\\";
      break;
    default:
      if (static_cast<unsigned char>(unescaped.str[i]) <= 0x1F) {
        // TODO can this be done once at the beginning, or will it mess up << char?
        std::ios::fmtflags f(out.flags());
        out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(unescaped.str[i]);
        out.flags(f);
      } else {
        out << unescaped.str[i];
      }
    }
  }
  return out;
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_JSONFORMATUTILS_H
