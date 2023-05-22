#define SIMDJSON_VERBOSE_LOGGING 1
#include "simdjson.h"
#include "test_ondemand.h"

#include <iostream>
#include <string>
#include <stdlib.h>

using namespace simdjson;

namespace log_error_tests {
#if SIMDJSON_EXCEPTIONS

using namespace std;

bool tape_error()
{
  TEST_START();
  auto json = R"( {"a", "hello"} )"_padded;
  ondemand::parser parser;
  try {
    ondemand::document doc = parser.iterate(json);
    std::cout << doc["a"] << std::endl;
    TEST_FAIL("Should have thrown an exception!")
  } catch (simdjson_error& e) {
    ASSERT_ERROR(e.error(), TAPE_ERROR);
  }
  TEST_SUCCEED();
}

bool no_such_field()
{
  TEST_START();
  auto json = R"( {"a": "hello"} )"_padded;
  ondemand::parser parser;
  try {
    ondemand::document doc = parser.iterate(json);
    std::cout << doc["missing_key"] << std::endl;
    TEST_FAIL("Should have thrown an exception!")
  } catch (simdjson_error& e) {
    ASSERT_ERROR(e.error(), NO_SUCH_FIELD);
  }
  TEST_SUCCEED();
}
#endif // SIMDJSON_EXCEPTIONS

bool run()
{
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::string str = "SIMDJSON_LOG_LEVEL=ERROR";
  putenv(str.data());
  bool rc =
#if SIMDJSON_EXCEPTIONS
            tape_error() &&
            no_such_field() &&
#endif // #if SIMDJSON_EXCEPTIONS
            true;
  SIMDJSON_POP_DISABLE_WARNINGS
  return rc;
}

}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, log_error_tests::run);
}
