#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace json_pointer_tests {
    bool easytest() {
        TEST_START();
        auto small_json = R"( [
            1.1,1.2,"BONJOUR"
         ] )"_padded;
        ondemand::parser parser;
        auto doc = parser.iterate(small_json);
        double x = doc.at_pointer("/0");
        std::cout << x << std::endl;
        TEST_SUCCEED();
    }

    bool run() {
        return
                easytest() &&
                true;
    }
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, json_pointer_tests::run);
}