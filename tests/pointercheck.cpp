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

const padded_string TEST_RFC_JSON = R"(
   {
      "foo": ["bar", "baz"],
      "": 0,
      "a/b": 1,
      "c%d": 2,
      "e^f": 3,
      "g|h": 4,
      "i\\j": 5,
      "k\"l": 6,
      " ": 7,
      "m~n": 8
   }
)"_padded;

bool json_pointer_success_test(const padded_string & source, const char *json_pointer, std::string_view expected_value) {
  std::cout << "Running successful JSON pointer test '" << json_pointer << "' ..." << std::endl;
  dom::parser parser;
  dom::element doc;
  auto error = parser.parse(source).get(doc);
  if(error) { std::cerr << "cannot parse: " << error << std::endl; return false; }
  dom::element answer;
  error = doc.at(json_pointer).get(answer);
  if(error) { std::cerr << "cannot access pointer: " << error << std::endl; return false; }
  std::string str_answer = simdjson::minify(answer);
  if(str_answer != expected_value) {
    std::cerr << "They differ!!!" << std::endl;
    std::cerr << "   found    '" << str_answer << "'" << std::endl;
    std::cerr << "   expected '" << expected_value << "'" << std::endl;
  }
  ASSERT_EQUAL(str_answer, expected_value);
  return true;
}

bool json_pointer_success_test(const padded_string & source, const char *json_pointer) {
  std::cout << "Running successful JSON pointer test '" << json_pointer << "' ..." << std::endl;
  dom::parser parser;
  ASSERT_SUCCESS( parser.parse(source).at(json_pointer).error() );
  return true;
}


bool json_pointer_failure_test(const padded_string & source, const char *json_pointer, error_code expected_error) {
  std::cout << "Running invalid JSON pointer test '" << json_pointer << "' ..." << std::endl;
  dom::parser parser;
  ASSERT_ERROR(parser.parse(source).at(json_pointer).error(), expected_error);
  return true;
}

int main() {
  if (true
    && json_pointer_success_test(TEST_RFC_JSON, "", R"({"foo":["bar","baz"],"":0,"a/b":1,"c%d":2,"e^f":3,"g|h":4,"i\\j":5,"k\"l":6," ":7,"m~n":8})")
    && json_pointer_success_test(TEST_RFC_JSON, "/foo", "[\"bar\",\"baz\"]")
    && json_pointer_success_test(TEST_RFC_JSON, "/foo/0", "\"bar\"")
    && json_pointer_success_test(TEST_RFC_JSON, "/", "0")
    && json_pointer_success_test(TEST_RFC_JSON, "/a~1b", "1")
    && json_pointer_success_test(TEST_RFC_JSON, "/c%d", "2")
    && json_pointer_success_test(TEST_RFC_JSON, "/e^f", "3")
    && json_pointer_success_test(TEST_RFC_JSON, "/g|h", "4")
    && json_pointer_success_test(TEST_RFC_JSON, "/i\\j", "5")
    && json_pointer_success_test(TEST_RFC_JSON, "/k\"l", "6")
    && json_pointer_success_test(TEST_RFC_JSON, "/ ", "7")
    && json_pointer_success_test(TEST_RFC_JSON, "/m~0n", "8")
    && json_pointer_success_test(TEST_JSON, "")
    && json_pointer_success_test(TEST_JSON, "~1~001abc")
    && json_pointer_success_test(TEST_JSON, "~1~001abc/1")
    && json_pointer_success_test(TEST_JSON, "~1~001abc/1/\\\" 0")
    && json_pointer_success_test(TEST_JSON, "~1~001abc/1/\\\" 0/0", "\"value0\"")
    && json_pointer_success_test(TEST_JSON, "~1~001abc/1/\\\" 0/1", "\"value1\"")
    && json_pointer_failure_test(TEST_JSON, "~1~001abc/1/\\\" 0/2", INDEX_OUT_OF_BOUNDS) // index actually out of bounds
    && json_pointer_success_test(TEST_JSON, "arr") // get array
    && json_pointer_failure_test(TEST_JSON, "arr/0", INDEX_OUT_OF_BOUNDS) // array index 0 out of bounds on empty array
    && json_pointer_success_test(TEST_JSON, "~1~001abc") // get object
    && json_pointer_success_test(TEST_JSON, "0", "\"0 ok\"") // object index with integer-ish key
    && json_pointer_success_test(TEST_JSON, "01", "\"01 ok\"") // object index with key that would be an invalid integer
    && json_pointer_success_test(TEST_JSON, "", R"({"/~01abc":[0,{"\\\" 0":["value0","value1"]}],"0":"0 ok","01":"01 ok","":"empty ok","arr":[]})") // object index with empty key
    && json_pointer_failure_test(TEST_JSON, "~01abc", NO_SUCH_FIELD) // Test that we don't try to compare the literal key
    && json_pointer_failure_test(TEST_JSON, "~1~001abc/01", INVALID_JSON_POINTER) // Leading 0 in integer index
    && json_pointer_failure_test(TEST_JSON, "~1~001abc/", INVALID_JSON_POINTER) // Empty index to array
    && json_pointer_failure_test(TEST_JSON, "~1~001abc/-", INDEX_OUT_OF_BOUNDS) // End index is always out of bounds
  ) {
    std::cout << "Success!" << std::endl;
    return 0;
  } else {
    std::cerr << "Failed!" << std::endl;
    return 1;
  }
}
