#include "simdjson.h"
#include "test_ondemand.h"
#include <string>

using namespace simdjson;

namespace number_in_string_tests {
    bool array_double() {
        TEST_START();
        auto json = R"(["1.2","2.3","-42.3","2.43442e3", "-1.234e3"])"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        size_t counter{0};
        std::vector<double> expected = {1.2, 2.3, -42.3, 2434.42, -1234};
        double d;
        for (auto value : doc) {
            ASSERT_SUCCESS(value.get_double_in_string().get(d));
            ASSERT_EQUAL(d,expected[counter++]);
        }
        TEST_SUCCEED();
    }

    bool array_int() {
        TEST_START();
        auto json = R"(["1", "2", "-3", "1000", "-7844"])"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        size_t counter{0};
        std::vector<int> expected = {1, 2, -3, 1000, -7844};
        int64_t i;
        for (auto value : doc) {
            ASSERT_SUCCESS(value.get_int64_in_string().get(i));
            ASSERT_EQUAL(i,expected[counter++]);
        }
        TEST_SUCCEED();
    }

    bool array_unsigned() {
        TEST_START();
        auto json = R"(["1", "2", "24", "9000", "156934"])"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        size_t counter{0};
        std::vector<int> expected = {1, 2, 24, 9000, 156934};
        uint64_t u;
        for (auto value : doc) {
            ASSERT_SUCCESS(value.get_uint64_in_string().get(u));
            ASSERT_EQUAL(u,expected[counter++]);
        }
        TEST_SUCCEED();
    }

    bool object() {
        TEST_START();
        auto json = R"({"a":"1.2", "b":"-2.342e2", "c":"22", "d":"-112358", "e":"1080", "f":"123456789"})"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        size_t counter{0};
        std::vector<double> expected = {1.2, -234.2, 22, -112358, 1080, 123456789};
        double d;
        int64_t i;
        uint64_t u;
        // Doubles
        ASSERT_SUCCESS(doc.find_field("a").get_double_in_string().get(d));
        ASSERT_EQUAL(d,expected[counter++]);
        ASSERT_SUCCESS(doc.find_field("b").get_double_in_string().get(d));
        ASSERT_EQUAL(d,expected[counter++]);
        // Integers
        ASSERT_SUCCESS(doc.find_field("c").get_int64_in_string().get(i));
        ASSERT_EQUAL(i,expected[counter++]);
        ASSERT_SUCCESS(doc.find_field("d").get_int64_in_string().get(i));
        ASSERT_EQUAL(i,expected[counter++]);
        // Unsigned integers
        ASSERT_SUCCESS(doc.find_field("e").get_uint64_in_string().get(u));
        ASSERT_EQUAL(u,expected[counter++]);
        ASSERT_SUCCESS(doc.find_field("f").get_uint64_in_string().get(u));
        ASSERT_EQUAL(u,expected[counter++]);
        TEST_SUCCEED();
    }

    bool docs() {
        TEST_START();
        auto double_doc = R"( "-1.23e1" )"_padded;
        auto int_doc = R"( "-243" )"_padded;
        auto uint_doc = R"( "212213" )"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        double d;
        int64_t i;
        uint64_t u;
        // Double
        ASSERT_SUCCESS(parser.iterate(double_doc).get(doc));
        ASSERT_SUCCESS(doc.get_double_in_string().get(d));
        ASSERT_EQUAL(d,-12.3);
        // Integer
        ASSERT_SUCCESS(parser.iterate(int_doc).get(doc));
        ASSERT_SUCCESS(doc.get_int64_in_string().get(i));
        ASSERT_EQUAL(i,-243);
        // Unsinged integer
        ASSERT_SUCCESS(parser.iterate(uint_doc).get(doc));
        ASSERT_SUCCESS(doc.get_uint64_in_string().get(u));
        ASSERT_EQUAL(u,212213);
        TEST_SUCCEED();
    }

    bool run() {
        return  array_double() &&
                array_int() &&
                array_unsigned() &&
                object() &&
                docs() &&
                true;
    }
}   // number_in_string_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, number_in_string_tests::run);
}