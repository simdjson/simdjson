#include <iostream>

#include "simdjson.h"
#include "test_macros.h"

// we define our own asserts to get around NDEBUG
#ifndef ASSERT
#define ASSERT(x)                                                       \
{    if (!(x)) {                                                        \
        std::cerr << "Failed assertion " << #x << std::endl;            \
        return false;                                                   \
    }                                                                   \
}
#endif

using namespace simdjson;

const padded_string TEST_JSON = R"(
  {
    "/~01abc": [
      0,
      {
        "\\\" 0": [
          "value0",
          "value1"
        ]
      }
    ],
    "0": "0 ok",
    "01": "01 ok",
    "": "empty ok",
    "arr": []
  }
)"_padded;

bool json_pointer_success_test(const char *json_pointer, std::string_view expected_value) {
  std::cout << "Running successful JSON pointer test '" << json_pointer << "' ..." << std::endl;
  dom::parser parser;
  std::string_view value;
  ASSERT_SUCCESS( parser.parse(TEST_JSON).at(json_pointer).get(value) );
  ASSERT_EQUAL(value, expected_value);
  return true;
}

bool json_pointer_success_test(const char *json_pointer) {
  std::cout << "Running successful JSON pointer test '" << json_pointer << "' ..." << std::endl;
  dom::parser parser;
  ASSERT_SUCCESS( parser.parse(TEST_JSON).at(json_pointer).error() );
  return true;
}


bool json_pointer_failure_test(const char *json_pointer, error_code expected_error) {
  std::cout << "Running invalid JSON pointer test '" << json_pointer << "' ..." << std::endl;
  dom::parser parser;
  ASSERT_ERROR(parser.parse(TEST_JSON).at(json_pointer).error(), expected_error);
  return true;
}

int main() {
  if (true
    && json_pointer_success_test("")
    && json_pointer_success_test("~1~001abc")
    && json_pointer_success_test("~1~001abc/1")
    && json_pointer_success_test("~1~001abc/1/\\\" 0")
    && json_pointer_success_test("~1~001abc/1/\\\" 0/0", "value0")
    && json_pointer_success_test("~1~001abc/1/\\\" 0/1", "value1")
    && json_pointer_failure_test("~1~001abc/1/\\\" 0/2", INDEX_OUT_OF_BOUNDS) // index actually out of bounds
    && json_pointer_success_test("arr") // get array
    && json_pointer_failure_test("arr/0", INDEX_OUT_OF_BOUNDS) // array index 0 out of bounds on empty array
    && json_pointer_success_test("~1~001abc") // get object
    && json_pointer_success_test("0", "0 ok") // object index with integer-ish key
    && json_pointer_success_test("01", "01 ok") // object index with key that would be an invalid integer
    && json_pointer_success_test("", "empty ok") // object index with empty key
    && json_pointer_failure_test("~01abc", NO_SUCH_FIELD) // Test that we don't try to compare the literal key
    && json_pointer_failure_test("~1~001abc/01", INVALID_JSON_POINTER) // Leading 0 in integer index
    && json_pointer_failure_test("~1~001abc/", INVALID_JSON_POINTER) // Empty index to array
    && json_pointer_failure_test("~1~001abc/-", INDEX_OUT_OF_BOUNDS) // End index is always out of bounds
  ) {
    std::cout << "Success!" << std::endl;
    return 0;
  } else {
    std::cerr << "Failed!" << std::endl;
    return 1;
  }
}
