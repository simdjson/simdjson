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
        std::string json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3])";
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

    bool simple_document_iteration_multiple_batches() {
        TEST_START();
        std::string json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3])";
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json,32).get(stream));
        std::string_view expected[4] = {"[1,[1,2]]", "{\"a\":1,\"b\":2}", "{\"o\":{\"1\":1,\"2\":2}}", "[1,2,3]"};
        size_t counter{0};
        for(auto i = stream.begin(); i != stream.end(); ++i) {
            ASSERT_TRUE(counter < 4);
            ASSERT_EQUAL(i.source(), expected[counter++]);
        }
        ASSERT_EQUAL(counter, 4);
        TEST_SUCCEED();
    }

    bool simple_document_iteration_with_parsing() {
        TEST_START();
        std::string json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3])";
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        std::string_view expected[4] = {"[1,[1,2]]", "{\"a\":1,\"b\":2}", "{\"o\":{\"1\":1,\"2\":2}}", "[1,2,3]"};
        size_t counter{0};
        auto i = stream.begin();
        int64_t x;

        ASSERT_EQUAL(i.source(),expected[counter++]);
        ASSERT_SUCCESS( (*i).at_pointer("/1/1").get(x) );
        ASSERT_EQUAL(x,2);
        ++i;

        ASSERT_EQUAL(i.source(),expected[counter++]);
        ASSERT_SUCCESS( (*i).find_field("a").get(x) );
        ASSERT_EQUAL(x,1);
        ++i;

        ASSERT_EQUAL(i.source(),expected[counter++]);
        ASSERT_SUCCESS( (*i).at_pointer("/o/2").get(x) );
        ASSERT_EQUAL(x,2);
        ++i;

        ASSERT_EQUAL(i.source(),expected[counter++]);
        ASSERT_SUCCESS( (*i).at_pointer("/2").get(x) );
        ASSERT_EQUAL(x,3);
        ++i;

        if (i != stream.end()) { return false; }

        TEST_SUCCEED();
    }

    bool atoms_json() {
        TEST_START();
        std::string json = R"(5 true 20.3 "string" )";
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
        std::string json = R"({"z":5}  {"1":1,"2":2,"4":4} [7,  10,   9]  [15,  11,   12, 13]  [154,  110,   112, 1311])";
        std::string_view expected[5] = {R"({"z":5})",R"({"1":1,"2":2,"4":4})","[7,  10,   9]","[15,  11,   12, 13]","[154,  110,   112, 1311]"};
        size_t expected_indexes[5] = {0, 9, 29, 44, 65};

        ondemand::parser parser;
        ondemand::document_stream stream;
        size_t counter{0};
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        for(auto i = stream.begin(); i != stream.end(); ++i) {
            ASSERT_TRUE(counter < 5);
            ASSERT_EQUAL(i.current_index(), expected_indexes[counter]);
            ASSERT_EQUAL(i.source(), expected[counter]);
            counter++;
        }
        ASSERT_EQUAL(counter, 5);
        TEST_SUCCEED();
    }

    bool doc_index_multiple_batches() {
        TEST_START();
        std::string json = R"({"z":5}  {"1":1,"2":2,"4":4} [7,  10,   9]  [15,  11,   12, 13]  [154,  110,   112, 1311])";
        std::string_view expected[5] = {R"({"z":5})",R"({"1":1,"2":2,"4":4})","[7,  10,   9]","[15,  11,   12, 13]","[154,  110,   112, 1311]"};
        size_t expected_indexes[5] = {0, 9, 29, 44, 65};

        ondemand::parser parser;
        ondemand::document_stream stream;
        size_t counter{0};
        ASSERT_SUCCESS(parser.iterate_many(json,32).get(stream));
        for(auto i = stream.begin(); i != stream.end(); ++i) {
            ASSERT_TRUE(counter < 5);
            ASSERT_EQUAL(i.current_index(), expected_indexes[counter]);
            ASSERT_EQUAL(i.source(), expected[counter]);
            counter++;
        }
        ASSERT_EQUAL(counter, 5);
        TEST_SUCCEED();
    }

    bool source_test() {
        TEST_START();
        std::string json = R"([1,[1,2]]     {"a":1,"b":2}      {"o":{"1":1,"2":2}}   [1,2,3] )";
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
        std::string json = R"([1,2,3]  {"1":1,"2":3,"4":4} [1,2  )";
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
        std::string json = R"([1,2,3]  {"1":1,"2":3,"4":4} [1,2]  )";
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
        std::string json = R"([1,2,3]  {"1":1,"2":3,"4":4} "intentionally unclosed string  )";
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
        std::string json = R"([1,2,3]  {"1":1,"2":3,"4":4} {"key":"intentionally unclosed string  )";
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

    bool large_window() {
        TEST_START();
    #if SIZE_MAX > 17179869184
        std::string json = R"({"error":[],"result":{"token":"xxx"}}{"error":[],"result":{"token":"xxx"}})";
        ondemand::parser parser;
        uint64_t window_size{17179869184}; // deliberately too big
        ondemand::document_stream stream;
        ASSERT_SUCCESS( parser.iterate_many(json, size_t(window_size)).get(stream) );
        auto i = stream.begin();
        ASSERT_ERROR(i.error(),CAPACITY);
    #endif
        TEST_SUCCEED();
    }

    bool test_leading_spaces() {
        TEST_START();
        std::string input = R"(                               [1,1] [1,2] [1,3]  [1,4] [1,5] [1,6] [1,7] [1,8] [1,9] [1,10] [1,11] [1,12] [1,13] [1,14] [1,15] )";
        size_t count{0};
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(input, 32).get(stream));
        for(auto i = stream.begin(); i != stream.end(); ++i) {
            ASSERT_SUCCESS(i.error());
            count++;
        }
        ASSERT_EQUAL(count,15);
        TEST_SUCCEED();
    }


    bool test_crazy_leading_spaces() {
        TEST_START();
        std::string input = R"(                                                                                                                                                           [1,1] [1,2] [1,3]  [1,4] [1,5] [1,6] [1,7] [1,8] [1,9] [1,10] [1,11] [1,12] [1,13] [1,14] [1,15] )";
        size_t count{0};
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(input, 32).get(stream));
        for(auto i = stream.begin(); i != stream.end(); ++i) {
            ASSERT_SUCCESS(i.error());
            count++;
        }
        ASSERT_EQUAL(count,15);
        TEST_SUCCEED();
    }

    bool adversarial_single_document() {
        TEST_START();
        std::string json = R"({"f[)";
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        size_t count{0};
        for (auto & doc : stream) {
            (void)doc;
            count++;
        }
        ASSERT_EQUAL(count,0);
        TEST_SUCCEED();
    }

    bool adversarial_single_document_array() {
        TEST_START();
        std::string json = R"(["this is an unclosed string ])";
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        size_t count{0};
        for (auto & doc : stream) {
            (void)doc;
            count++;
        }
        ASSERT_EQUAL(count,0);
        TEST_SUCCEED();
    }

    bool document_stream_test() {
        TEST_START();
        fflush(NULL);
        const size_t n_records = 100;
        std::string data;
        std::vector<char> buf(1024);
        // Generating data
        for (size_t i = 0; i < n_records; ++i) {
            size_t n = snprintf(buf.data(),
                                buf.size(),
                            "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                            "\"ete\": {\"id\": %zu, \"name\": \"eventail%zu\"}}",
                            i, i, (i % 2) ? "homme" : "femme", i % 10, i % 10);
            if (n >= buf.size()) { abort(); }
            data += std::string(buf.data(), n);
        }

        for(size_t batch_size = 1000; batch_size < 2000; batch_size += (batch_size>1050?10:1)) {
            fflush(NULL);
            simdjson::padded_string str(data);
            ondemand::parser parser;
            ondemand::document_stream stream;
            size_t count{0};
            ASSERT_SUCCESS( parser.iterate_many(str, batch_size).get(stream) );
            for (auto & doc : stream) {
                int64_t keyid;
                ASSERT_SUCCESS( doc["id"].get(keyid) );
                ASSERT_EQUAL( keyid, int64_t(count) );

                count++;
            }
            ASSERT_EQUAL(count,n_records);
        }
        TEST_SUCCEED();
    }


    bool document_stream_utf8_test() {
        TEST_START();
        fflush(NULL);
        const size_t n_records = 100;
        std::string data;
        std::vector<char> buf(1024);
        // Generating data
        for (size_t i = 0; i < n_records; ++i) {
        size_t n = snprintf(buf.data(),
                            buf.size(),
                        "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                        "\"\xC3\xA9t\xC3\xA9\": {\"id\": %zu, \"name\": \"\xC3\xA9ventail%zu\"}}",
                        i, i, (i % 2) ? "\xE2\xBA\x83" : "\xE2\xBA\x95", i % 10, i % 10);
        if (n >= buf.size()) { abort(); }
        data += std::string(buf.data(), n);
        }

        for(size_t batch_size = 1000; batch_size < 2000; batch_size += (batch_size>1050?10:1)) {
            fflush(NULL);
            simdjson::padded_string str(data);
            ondemand::parser parser;
            ondemand::document_stream stream;
            size_t count{0};
            ASSERT_SUCCESS( parser.iterate_many(str, batch_size).get(stream) );
            for (auto & doc : stream) {
                int64_t keyid;
                ASSERT_SUCCESS( doc["id"].get(keyid) );
                ASSERT_EQUAL( keyid, int64_t(count) );

                count++;
            }
            ASSERT_EQUAL( count, n_records )
        }
        TEST_SUCCEED();
    }

    bool stress_data_race() {
    TEST_START();
    // Correct JSON.
    std::string input = R"([1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] )";
    ondemand::parser parser;
    ondemand::document_stream stream;
    ASSERT_SUCCESS(parser.iterate_many(input, 32).get(stream));
    for(auto i = stream.begin(); i != stream.end(); ++i) {
      ASSERT_SUCCESS(i.error());
    }
    TEST_SUCCEED();
  }

  bool stress_data_race_with_error() {
    TEST_START();
    #if SIMDJSON_THREAD_ENABLED
    std::cout << "ENABLED" << std::endl;
    #endif
    // Intentionally broken
    std::string input = R"([1,23] [1,23] [1,23] [1,23 [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] )";
    ondemand::parser parser;
    ondemand::document_stream stream;
    ASSERT_SUCCESS(parser.iterate_many(input, 32).get(stream));
    size_t count{0};
    for(auto i = stream.begin(); i != stream.end(); ++i) {
        auto error = i.error();
        if(count <= 3) {
            ASSERT_SUCCESS(error);
        } else {
            ASSERT_ERROR(error,TAPE_ERROR);
            break;
        }
        count++;
    }
    TEST_SUCCEED();
  }

    bool run() {
        return
            simple_document_iteration() &&
            simple_document_iteration_multiple_batches() &&
            simple_document_iteration_with_parsing() &&
            atoms_json() &&
            doc_index() &&
            doc_index_multiple_batches() &&
            source_test() &&
            truncated() &&
            truncated_complete_docs() &&
            truncated_unclosed_string() &&
            small_window() &&
            large_window() &&
            test_leading_spaces() &&
            test_crazy_leading_spaces() &&
            adversarial_single_document() &&
            adversarial_single_document_array() &&
            document_stream_test() &&
            document_stream_utf8_test() &&
            stress_data_race() &&
            stress_data_race_with_error() &&
            true;
    }
} // document_stream_tests

int main (int argc, char *argv[]) {
    return test_main(argc, argv, document_stream_tests::run);
}