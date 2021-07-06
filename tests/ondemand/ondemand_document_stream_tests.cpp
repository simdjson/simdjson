#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace document_stream_tests {

    std::string my_string(ondemand::document& doc) {
        std::stringstream ss;
        ss << doc;
        return ss.str();
    }

    bool simple_document_iteration() {
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

    bool atoms_json() {
        TEST_START();
        auto json = R"(5 true 20.3 "string" )"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));

        std::string_view expected[4] = {"5", "true", "20.3", "\"string\""};
        size_t counter{0};
        for (auto i = stream.begin(); i != stream.end(); ++i) {
            ASSERT_EQUAL(i.source(), expected[counter++]);
        }
        ASSERT_EQUAL(counter,4);
        TEST_SUCCEED();
    }

    bool doc_index() {
        TEST_START();
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
    }

    bool doc_index_multiple_batches() {
        TEST_START();
        auto json = R"({"z":5}  {"1":1,"2":2,"4":4} [7,  10,   9]  [15,  11,   12, 13]  [154,  110,   112, 1311])"_padded;
        std::string_view expected[5] = {"{\"z\":5}", "{\"1\":1,\"2\":2,\"4\":4}", "[7,10,9]", "[15,11,12,13]", "[154,110,112,1311]"};
        size_t expected_indexes[5] = {0, 9, 29, 44, 65};

        ondemand::parser parser;
        ondemand::document_stream stream;
        size_t counter{0};
        ASSERT_SUCCESS(parser.iterate_many(json,32).get(stream));
        for(auto i = stream.begin(); i != stream.end(); ++i) {
            ASSERT_TRUE(counter < 5);
            ASSERT_EQUAL(i.current_index(), expected_indexes[counter]);
            ASSERT_EQUAL(my_string(*i), expected[counter]);
            counter++;
        }
        ASSERT_EQUAL(counter, 5);
        TEST_SUCCEED();
    }

    bool testing_source() {
        TEST_START();
        // TODO: Add test where source is called after document is parsed
        auto json = R"([1,[1,2]]     {"a":1,"b":2}      {"o":{"1":1,"2":2}}   [1,2,3] )"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        std::string_view expected[4] = {"[1,[1,2]]", "{\"a\":1,\"b\":2}", "{\"o\":{\"1\":1,\"2\":2}}", "[1,2,3]"};
        size_t counter{0};
        for (auto i = stream.begin(); i != stream.end(); ++i) {
            ASSERT_EQUAL(i.source(), expected[counter++]);
        }
        ASSERT_EQUAL(counter,4);
        TEST_SUCCEED();
    }

    bool truncated() {
        TEST_START();
        // The last JSON document is intentionally truncated.
        auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} [1,2  )"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));

        size_t counter{0};
        for (auto i = stream.begin(); i != stream.end(); ++i) {
            counter++;
        }
        size_t truncated = stream.truncated_bytes();
        ASSERT_EQUAL(truncated, 6);
        ASSERT_EQUAL(counter,2);
        TEST_SUCCEED();
    }

    bool truncated_complete_docs() {
        TEST_START();
        auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} [1,2]  )"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));

        size_t counter{0};
        for (auto i = stream.begin(); i != stream.end(); ++i) {
            counter++;
        }
        size_t truncated = stream.truncated_bytes();
        ASSERT_EQUAL(truncated, 0);
        ASSERT_EQUAL(counter,3);
        TEST_SUCCEED();
    }

    bool truncated_unclosed_string() {
        TEST_START();
        // The last JSON document is intentionally truncated.
        auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} "intentionally unclosed string  )"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        // We use a window of json.size() though any large value would do.
        ASSERT_SUCCESS( parser.iterate_many(json).get(stream) );
        size_t counter{0};
        for(auto i = stream.begin(); i != stream.end(); ++i) {
            counter++;
        }
        size_t truncated = stream.truncated_bytes();
        ASSERT_EQUAL(counter,2);
        ASSERT_EQUAL(truncated,32);
        TEST_SUCCEED();
    }

    bool truncated_unclosed_string_in_object() {
    // The last JSON document is intentionally truncated.
    auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} {"key":"intentionally unclosed string  )"_padded;
    ondemand::parser parser;
    ondemand::document_stream stream;
    ASSERT_SUCCESS( parser.iterate_many(json).get(stream) );
    size_t counter{0};
    for(auto i = stream.begin(); i != stream.end(); ++i) {
        counter++;
    }
    size_t truncated = stream.truncated_bytes();
    ASSERT_EQUAL(counter,2);
    ASSERT_EQUAL(truncated,39);
    TEST_SUCCEED();
    }

    bool small_window() {
        TEST_START();
        std::vector<char> input;
        input.push_back('[');
        for(size_t i = 1; i < 1024; i++) {
            input.push_back('1');
            input.push_back(i < 1023 ? ',' : ']');
        }
        auto json = simdjson::padded_string(input.data(),input.size());
        ondemand::parser parser;
        size_t window_size{1024}; // deliberately too small
        ondemand::document_stream stream;
        ASSERT_SUCCESS( parser.iterate_many(json, window_size).get(stream) );
        auto i = stream.begin();
        ASSERT_ERROR(i.error(), CAPACITY);
        TEST_SUCCEED();
    }

    bool run() {
        return
            simple_document_iteration() &&
            atoms_json() &&
            doc_index() &&
            doc_index_multiple_batches() &&
            testing_source() &&
            truncated() &&
            truncated_complete_docs() &&
            truncated_unclosed_string() &&
            small_window() &&
            true;
    }
} // document_stream_tests

int main (int argc, char *argv[]) {
    return test_main(argc, argv, document_stream_tests::run);
}