#define SIMDJSON_VERBOSE_LOGGING 1
#include "simdjson.h"
#include "test_ondemand.h"

#include <iostream>

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
  setenv("SIMDJSON_LOG_LEVEL", "ERROR", 1);
  bool rc =
#if SIMDJSON_EXCEPTIONS
            tape_error() &&
            no_such_field() &&
#endif // #if SIMDJSON_EXCEPTIONS
            true;
  unsetenv("SIMDJSON_LOG_LEVEL");
  return rc;
}

}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, log_error_tests::run);
}
