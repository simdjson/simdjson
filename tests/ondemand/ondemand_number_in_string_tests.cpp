#include "simdjson.h"
#include "test_ondemand.h"
#include <string>

using namespace simdjson;

namespace number_in_string_tests {
    bool array() {
        TEST_START();
        auto json = R"(["1.2","2.3"])"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        for (auto value : doc) {
            std::cout << value.get_double_from_string() << std::endl;
        }
        TEST_SUCCEED();
    }

    bool object() {
        TEST_START();
        auto json = R"({"a":"1.2", "b":"2.3"})"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        std::cout << doc.find_field("a").get_double_from_string() << std::endl;
        TEST_SUCCEED();
    }

    bool run() {
        return  array() &&
                object() &&
                true;
    }
}   // number_in_string_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, number_in_string_tests::run);
}