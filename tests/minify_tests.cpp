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

int main() {
  return minify_test() && integer_test() ? EXIT_SUCCESS : EXIT_FAILURE;
}
