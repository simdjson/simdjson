#include <cinttypes>
#include <ciso646>
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

#include "simdjson.h"
#include "test_ondemand.h"
namespace tostring_tests {
#if SIMDJSON_EXCEPTIONS


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
  simdjson::ondemand::parser parser;
  std::cout << "Loading " << filename << std::endl;
  simdjson::padded_string docdata;
  auto error = simdjson::padded_string::load(filename).get(docdata);
  if(error) {
      std::cerr << "could not load " << filename << " got " << error << std::endl;
      return false;
  }  
  simdjson::ondemand::document doc;
  error = parser.iterate(docdata).get(doc);
  if (error) { std::cerr << error << std::endl; return false; }
  auto serial1 = simdjson::to_string(doc);
  serial1.reserve(serial1.size() + simdjson::SIMDJSON_PADDING);
  error = parser.iterate(serial1).get(doc);
  if (error) { std::cerr << error << std::endl; return false; }
  auto serial2 = simdjson::to_string(doc);
  bool match = (serial1 == serial2);
  if (match) {
    std::cout << "Parsing to_string and calling to_string again results in the "
                 "same content."
              << "Got " << serial1.size() << " bytes."
              << std::endl;
  }
  return match;
}


bool minify_test() {
  TEST_START();
  for (size_t i = 0; i < sizeof(test_files) / sizeof(test_files[0]); i++) {
    bool ok = load_to_string(test_files[i]);
    if (!ok) {
      return false;
    }
  }
  return true;
}
#endif // SIMDJSON_EXCEPTIONS

  bool run() {
    return
#if SIMDJSON_EXCEPTIONS
      minify_test() &&
      // parser_doc_limits() && // Failure is dependent on build type here ...
#endif // SIMDJSON_EXCEPTIONS
      true;
  }

} // namespace tostring_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, tostring_tests::run);
}

