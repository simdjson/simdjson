#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace document_stream_tests {

    std::string my_string(ondemand::document& doc) {
        std::stringstream ss;
        ss << doc;
        return ss.str();
    }

    bool testing_document_iteration() {
        TEST_START();
        auto json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3])"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        std::string_view expected[4] = {"[1,[1,2]]", "{\"a\":1,\"b\":2}", "{\"o\":{\"1\":1,\"2\":2}}", "[1,2,3]"};
        size_t counter{0};
        for(auto & doc : stream) {
            ASSERT_TRUE(counter < 4);
            ASSERT_EQUAL(my_string(doc), expected[counter++]);
        }
        ASSERT_EQUAL(counter, 4);
        TEST_SUCCEED();
    }

    bool doc_index() {
        TEST_START();
        // TODO: Check if works when multiple batches are loaded
        auto json = R"({"z":5}  {"1":1,"2":2,"4":4} [7,  10,   9]  [15,  11,   12, 13]  [154,  110,   112, 1311])"_padded;
        std::string_view expected[5] = {"{\"z\":5}", "{\"1\":1,\"2\":2,\"4\":4}", "[7,10,9]", "[15,11,12,13]", "[154,110,112,1311]"};
        size_t expected_indexes[5] = {0, 9, 29, 44, 65};

        ondemand::parser parser;
        ondemand::document_stream stream;
        size_t counter{0};
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        for(auto i = stream.begin(); i != stream.end(); ++i) {
            ASSERT_TRUE(counter < 5);
            ASSERT_EQUAL(i.current_index(), expected_indexes[counter]);
            ASSERT_EQUAL(my_string(*i), expected[counter]);
            counter++;
        }
        ASSERT_EQUAL(counter, 5);
        TEST_SUCCEED();
        TEST_SUCCEED();
    }

    bool testing_source() {
        TEST_START();
        // TODO: Add test where source is called after document is parsed
        auto json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3] )"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        std::string_view expected[4] = {"[1,[1,2]]", "{\"a\":1,\"b\":2}", "{\"o\":{\"1\":1,\"2\":2}}", "[1,2,3]"};
        size_t counter{0};
        for (auto i = stream.begin(); i != stream.end(); ++i) {
            ASSERT_EQUAL(i.source(), expected[counter++]);
        }
        TEST_SUCCEED();
    }

    bool run() {
        return
            testing_document_iteration() &&
            doc_index() &&
            testing_source() &&
            true;
    }
} // document_stream_tests

int main (int argc, char *argv[]) {
    return test_main(argc, argv, document_stream_tests::run);
}