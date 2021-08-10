#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace error_location_tests {

    bool testing() {
        TEST_START();
        TEST_SUCCEED();
    }

    bool run() {
        return  testing() &&
                true;
    }

} // namespace error_location_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, error_location_tests::run);
}