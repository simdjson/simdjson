#ifndef SIMDJSON_MINIFY_H
#define SIMDJSON_MINIFY_H

#include "simdjson/common_defs.h"
#include "simdjson/padded_string.h"
#include <string>
#include <ostream>
#include <sstream>

namespace simdjson {



/**
 *
 * Minify the input string assuming that it represents a JSON string, does not parse or validate.
 * This function is much faster than parsing a JSON string and then writing a minified version of it.
 * However, it does not validate the input. It will merely return an error in simple cases (e.g., if 
 * there is a string that was never terminated).
 *
 *
 * @param buf the json document to minify.
 * @param len the length of the json document.
 * @param dst the buffer to write the minified document to. *MUST* be allocated up to len bytes.
 * @param dst_len the number of bytes written. Output only.
 * @return the error code, or SUCCESS if there was no error.
 */
SIMDJSON_WARN_UNUSED error_code minify(const char *buf, size_t len, char *dst, size_t &dst_len) noexcept;

/**
 * Minifies a JSON element or document, printing the smallest possible valid JSON.
 *
 *   dom::parser parser;
 *   element doc = parser.parse("   [ 1 , 2 , 3 ] "_padded);
 *   cout << minify(doc) << endl; // prints [1,2,3]
 *
 */
template<typename T>
class minifier {
public:
  /**
   * Create a new minifier.
   *
   * @param _value The document or element to minify.
   */
  inline minifier(const T &_value) noexcept : value{_value} {}

  /**
   * Minify JSON to a string.
   */
  inline operator std::string() const noexcept { std::stringstream s; s << *this; return s.str(); }

  /**
   * Minify JSON to an output stream.
   */
  inline std::ostream& print(std::ostream& out);
private:
  const T &value;
};

template<typename T>
inline minifier<T> minify(const T &value) noexcept { return minifier<T>(value); }

/**
 * Minify JSON to an output stream.
 *
 * @param out The output stream.
 * @param formatter The minifier.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
template<typename T>
inline std::ostream& operator<<(std::ostream& out, minifier<T> formatter) { return formatter.print(out); }

} // namespace simdjson

#endif // SIMDJSON_MINIFY_H