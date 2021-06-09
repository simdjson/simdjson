#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace json_pointer_tests {
    bool easytest() {
        TEST_START();
        auto small_json = R"( [ 40.1, 39.9, 37.7, 40.4 ] )"_padded;
        ondemand::parser parser;
        auto doc = parser.iterate(small_json);
        double x = doc.at_pointer("/0");
        std::cout << x << std::endl;

        auto small_json2 = R"( { "make": [1,2,3], "model": "Camry",  "year": 2018 } )"_padded;
        auto doc2 = parser.iterate(small_json2);
        auto y = doc2.at_pointer("/make");
        std::cout << y << std::endl;
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