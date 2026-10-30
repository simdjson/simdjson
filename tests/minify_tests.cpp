#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "cast_tester.h"
#include "simdjson.h"
#include "test_macros.h"

const char *test_files[] = {
    TWITTER_JSON, TWITTER_TIMELINE_JSON, REPEAT_JSON, CANADA_JSON,
    MESH_JSON,    APACHE_JSON,           GSOC_JSON};
/**
 * The general idea of these tests if that if you take a JSON file,
 * load it, then convert it into a string, then parse that, and
 * convert it again into a second string, then the two strings should
 * be  identifical. If not, then something was lost or added in the
 * process.
 */

bool load_to_string(const char *filename) {
  std::cout << "Loading " << filename << std::endl;
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.load(filename).get(doc);
  if (error) { std::cerr << error << std::endl; return false; }
  auto serial1 = simdjson::to_string(doc);
  error = parser.parse(serial1).get(doc);
  if (error) { std::cerr << error << std::endl; return false; }
  auto serial2 = simdjson::to_string(doc);
  bool match = (serial1 == serial2);
  if (match) {
    std::cout << "Parsing to_string and calling to_string again results in the "
                 "same content."
              << std::endl;
  } else {
    std::cout << "The content differs!" << std::endl;
  }
  return match;
}

bool load_minify(const char *filename) {
  std::cout << "Loading " << filename << std::endl;
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.load(filename).get(doc);
  if (error) { std::cerr << error << std::endl; return false; }
  auto serial1 = simdjson::minify(doc);
  error = parser.parse(serial1).get(doc);
  if (error) { std::cerr << error << std::endl; return false; }
  auto serial2 = simdjson::minify(doc);
  bool match = (serial1 == serial2);
  if (match) {
    std::cout << "Parsing minify and calling minify again results in the same "
                 "content."
              << std::endl;
  } else {
    std::cout << "The content differs!" << std::endl;
  }
  return match;
}

bool minify_test() {
  std::cout << "Running " << __func__ << std::endl;

  for (size_t i = 0; i < sizeof(test_files) / sizeof(test_files[0]); i++) {
    bool ok = load_to_string(test_files[i]) && load_minify(test_files[i]);
    if (!ok) {
      return false;
    }
  }
  return true;
}

// Each integer at a digit-count boundary, and the extremes, must come back
// unchanged from minify.
bool integer_test() {
  std::cout << "Running " << __func__ << std::endl;
  std::vector<std::string> numbers = {"0", "-1", "-9223372036854775808",
                                      "9223372036854775807",
                                      "18446744073709551615"};
  uint64_t power = 1;
  for (int digits = 1; digits <= 19; digits++) {
    numbers.push_back(std::to_string(power));
    numbers.push_back(std::to_string(power - 1));
    // "-" + std::to_string(...) inserts into a temporary and trips GCC bug
    // 105329 (-Wrestrict) at -O3 on the Ubuntu 22.04 GCC 12 job.
    std::string negative("-");
    negative += std::to_string(power);
    numbers.push_back(std::move(negative));
    power *= 10;
  }
  numbers.push_back(std::to_string(power));
  numbers.push_back(std::to_string(power - 1));
  simdjson::dom::parser parser;
  for (const std::string &number : numbers) {
    std::string json = "[" + number + "]";
    simdjson::dom::element doc;
    auto error = parser.parse(json).get(doc);
    if (error) {
      std::cerr << "could not parse " << json << ": " << error << std::endl;
      return false;
    }
    std::string minified = simdjson::minify(doc);
    if (minified != json) {
      std::cerr << "minify(" << json << ") gave " << minified << std::endl;
      return false;
    }
  }
  return true;
}

// The escaping that a JSON string needs, one byte at a time.
std::string escape_reference(const std::string &s) {
  static const char hex[] = "0123456789abcdef";
  std::string out = "\"";
  for (unsigned char c : s) {
    switch (c) {
    case '"': out += "\\\""; break;
    case '\\': out += "\\\\"; break;
    case '\b': out += "\\b"; break;
    case '\t': out += "\\t"; break;
    case '\n': out += "\\n"; break;
    case '\f': out += "\\f"; break;
    case '\r': out += "\\r"; break;
    default:
      if (c < 0x20) {
        out += "\\u00";
        out += hex[c >> 4];
        out += hex[c & 0xf];
      } else {
        out += char(c);
      }
    }
  }
  return out + "\"";
}

// Escapes s from a buffer of exactly s.size() bytes, so that under the address
// sanitizer a read past the end of the string fails the test.
bool check_escape(const std::string &s) {
  std::unique_ptr<char[]> exact(new char[s.size()]);
  std::memcpy(exact.get(), s.data(), s.size());
  simdjson::internal::mini_formatter formatter;
  formatter.string(std::string_view(exact.get(), s.size()));
  const std::string expected = escape_reference(s);
  if (formatter.str() != expected) {
    std::cerr << "escaping a string of " << s.size() << " bytes gave "
              << formatter.str() << ", expected " << expected << std::endl;
    return false;
  }
  return true;
}

// Strings of every length up to 48 bytes, with a character that needs
// escaping (or one that does not) at every position, alone and next to a
// second one at the end; then strings made only of such characters, including
// one where every byte becomes \u0001.
bool string_escape_test() {
  std::cout << "Running " << __func__ << std::endl;
  const std::string specials("\"\\\n\x00\x01\x1f\x20\x7f\x80\xff", 10);
  for (size_t len = 0; len <= 48; len++) {
    if (!check_escape(std::string(len, 'a'))) { return false; }
    for (char c : specials) {
      for (size_t pos = 0; pos < len; pos++) {
        std::string s(len, 'a');
        s[pos] = c;
        if (!check_escape(s)) { return false; }
        s[len - 1] = '"';
        if (!check_escape(s)) { return false; }
      }
    }
    std::string dense;
    for (size_t i = 0; i < len; i++) { dense += specials[i % 3]; }
    if (!check_escape(dense)) { return false; }
    // Every byte becomes six: the most output per input byte.
    if (!check_escape(std::string(len, '\x01'))) { return false; }
  }
  // A run of bytes that become six each, starting at every distance from the
  // end of the output buffer, so that writing more than was reserved would
  // overflow it (and fail under the address sanitizer).
  for (size_t prefix = 0; prefix <= 300; prefix++) {
    if (!check_escape(std::string(prefix, 'a') + std::string(40, '\x01'))) { return false; }
  }
  // The same bytes through a parse and minify.
  simdjson::dom::parser parser;
  for (size_t len = 0; len <= 40; len++) {
    std::string s;
    for (size_t i = 0; i < len; i++) { s += (i % 7 == 3) ? '\n' : (i % 11 == 5) ? '"' : 'x'; }
    // Not "[" + escape_reference(s): GCC bug 105329 (-Wrestrict), as above.
    std::string json = "[";
    json += escape_reference(s);
    json += ']';
    simdjson::dom::element doc;
    auto error = parser.parse(json).get(doc);
    if (error) {
      std::cerr << "could not parse " << json << ": " << error << std::endl;
      return false;
    }
    const std::string minified = simdjson::minify(doc);
    if (minified != json) {
      std::cerr << "minify(" << json << ") gave " << minified << std::endl;
      return false;
    }
  }
  return true;
}

int main() {
  return minify_test() && integer_test() && string_escape_test() ? EXIT_SUCCESS : EXIT_FAILURE;
}
