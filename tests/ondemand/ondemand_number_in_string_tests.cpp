#include "simdjson.h"
#include "test_ondemand.h"
#include <string>

using namespace simdjson;

namespace number_in_string_tests {
    bool array() {
        TEST_START();
        auto json = R"(["1.2","2.3","-42.3","2.43442e3", 5.432, -23.2])"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        size_t counter{0};
        std::vector<double> expected = {1.2, 2.3,-42.3,2434.42, 5.432, -23.2};
        double d;
        for (auto value : doc) {
            d = value.get_double();
            ASSERT_EQUAL(d,expected[counter++]);
        }
        TEST_SUCCEED();
    }

    bool object() {
        TEST_START();
        auto json = R"({"a":"1.2", "b":"2.3", "c":"2.342e2", "d":"-2.25e1"})"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        size_t counter{0};
        std::vector<double> expected = {1.2, 2.3, 234.2, -22.5};
        double d;
        ASSERT_SUCCESS(doc.find_field("a").get_double().get(d));
        ASSERT_EQUAL(d,expected[counter++]);
        ASSERT_SUCCESS(doc.find_field("b").get_double().get(d));
        ASSERT_EQUAL(d,expected[counter++]);
        ASSERT_SUCCESS(doc.find_field("c").get_double().get(d));
        ASSERT_EQUAL(d,expected[counter++]);
        ASSERT_SUCCESS(doc.find_field("d").get_double().get(d));
        ASSERT_EQUAL(d,expected[counter++]);
        TEST_SUCCEED();
    }

    bool doc() {
        TEST_START();
        auto json = R"( "-1.23e1" )"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        double d = doc.get_double();
        ASSERT_EQUAL(d,-12.3);
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