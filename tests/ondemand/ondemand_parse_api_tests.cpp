#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace parse_api_tests {
  using namespace std;

  const padded_string BASIC_JSON = "[1,2,3]"_padded;
  const padded_string BASIC_NDJSON = "[1,2,3]\n[4,5,6]"_padded;
  const padded_string EMPTY_NDJSON = ""_padded;

  bool parser_iterate() {
    TEST_START();
    ondemand::parser parser;
    auto doc = parser.iterate(BASIC_JSON);
    ASSERT_SUCCESS( doc.get_array() );
    return true;
  }

  bool parser_iterate_padded_string_view() {
    TEST_START();
    ondemand::parser parser;

    padded_string_view json("12                                ", 2);
    auto doc = parser.iterate(json);
    ASSERT_SUCCESS( doc.get_double() );

    json = padded_string_view(std::string_view("12                                ", 2));
    doc = parser.iterate(json);
    ASSERT_SUCCESS( doc.get_double() );

    doc = parser.iterate(padded_string_view("12                                ", 2));
    ASSERT_SUCCESS( doc.get_double() );
    return true;
  }

  bool parser_iterate_promise_padded() {
    TEST_START();
    ondemand::parser parser;

    auto doc = parser.iterate(promise_padded("12                                ", 2));
    ASSERT_SUCCESS( doc.get_double() );

    std::string_view json("12                                ", 2);
    doc = parser.iterate(promise_padded(json));
    ASSERT_SUCCESS( doc.get_double() );

    return true;
  }

#if SIMDJSON_EXCEPTIONS
  bool parser_iterate_exception() {
    TEST_START();
    ondemand::parser parser;
    auto doc = parser.iterate(BASIC_JSON);
    simdjson_unused ondemand::array array = doc;
    return true;
  }
#endif // SIMDJSON_EXCEPTIONS

  bool run() {
    return parser_iterate() &&
           parser_iterate_padded_string_view() &&
           parser_iterate_promise_padded() &&
#if SIMDJSON_EXCEPTIONS
           parser_iterate_exception() &&
#endif // SIMDJSON_EXCEPTIONS
           true;
  }
}


int main(int argc, char *argv[]) {
  return test_main(argc, argv, parse_api_tests::run);
}
