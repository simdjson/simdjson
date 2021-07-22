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
        size_t counter{0};
        std::vector<double> expected = {1.2, 2.3};
        double d;
        for (auto value : doc) {
            d = value.get_double_from_string();
            ASSERT_EQUAL(d,expected[counter++]);
        }
        TEST_SUCCEED();
    }

    bool object() {
        TEST_START();
        auto json = R"({"a":"1.2", "b":"2.3"})"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        size_t counter{0};
        std::vector<double> expected = {1.2, 2.3};
        double d;
        d = doc.find_field("a").get_double_from_string();
        ASSERT_EQUAL(d,expected[counter++]);
        d = doc.find_field("b").get_double_from_string();
        ASSERT_EQUAL(d,expected[counter++]);
        TEST_SUCCEED();
    }

    bool doc() {
        TEST_START();
         auto json = R"( "1.2" )"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        double d = doc.get_double_from_string();
        ASSERT_EQUAL(d,1.2);
        TEST_SUCCEED();
    }

    bool run() {
        return  array() &&
                object() &&
                doc() &&
                true;
    }
}   // number_in_string_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, number_in_string_tests::run);
}