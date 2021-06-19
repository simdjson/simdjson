#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace parse_api_tests {
  using namespace std;

  const std::string_view BASIC_JSON = "[1,2,3]";

  bool parser_iterate_empty() {
    TEST_START();
    FILE *p;
    // Of course, we could just call iterate on the empty string, but
    // we want to test the whole process.
    const char *const tmpfilename = "emptyondemand.txt";
    if((p = fopen(tmpfilename, "w")) != nullptr) {
      fclose(p);
      auto json = padded_string::load(tmpfilename);
      ondemand::document doc;
      ondemand::parser parser;
      auto error = parser.iterate(json).get(doc);
      remove(tmpfilename);
      if(error != simdjson::EMPTY) {
        std::cerr << "Was expecting empty but got " << error << std::endl;
        return false;
      }
    } else {
      std::cout << "Warning: I could not create temporary file " << tmpfilename << std::endl;
      std::cout << "We omit testing the empty file case." << std::endl;
    }
    return true;
  }

  bool parser_iterate() {
    TEST_START();
    ondemand::parser parser;
    cout << "iterating" << endl;
    auto doc = parser.iterate(BASIC_JSON);
    cout << "iterated" << endl;
    ASSERT_SUCCESS( doc.get_array() );
    cout << "got array" << endl;
    return true;
  }

#if SIMDJSON_EXCEPTIONS
  bool parser_iterate_exception() {
    TEST_START();
    ondemand::parser parser;
    auto doc = parser.iterate(BASIC_JSON);
    simdjson_unused ondemand::array array = doc;
    TEST_SUCCEED();
  }
#endif // SIMDJSON_EXCEPTIONS

  bool run() {
    return //parser_iterate_empty() &&
           parser_iterate() &&
#if SIMDJSON_EXCEPTIONS
           parser_iterate_exception() &&
#endif // SIMDJSON_EXCEPTIONS
           true;
  }
}


int main(int argc, char *argv[]) {
  return test_main(argc, argv, parse_api_tests::run);
}
