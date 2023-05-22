#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
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

int main() { return minify_test() ? EXIT_SUCCESS : EXIT_FAILURE; }
