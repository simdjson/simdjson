#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace document_stream_tests {

    template <typename T>
    bool process_doc(T &docref) {
        int64_t val{};
        ASSERT_SUCCESS(docref.at_pointer("/4").get(val));
        ASSERT_EQUAL(val, 5);
        return true;
    }
    bool truncated_utf8() {
        TEST_START();
        // truncated UTF-8
        auto json = "\"document1\" \xC3"_padded;

        ondemand::parser parser;
        ondemand::document_stream stream;
        auto oderror = parser.iterate_many(json).get(stream);
        if (oderror) {
          std::cerr << "ondemand iterate_many error: " << oderror << std::endl;
          return false;
        }
        int document_index = 0;
        auto i = stream.begin();
        for (; i != stream.end(); ++i) {
            ondemand::document_reference doc;
            auto err = (*i).get(doc);
            document_index++;
            // With the fallback routine, we might detect a 'document' since
            // it does not do UTF-8 validation in stage one, but it will do
            // so when we access the document.
            if(err == SUCCESS) {
                ondemand::json_type t;
                err = doc.type().get(t);
            }
            if((err == SUCCESS) && (document_index != 1)) {
              std::cerr << "Only the first document should be valid." << std::endl;
              return false;
            }
            if((err != SUCCESS) && (document_index != 2)) {
              std::cerr << "ondemand iterate_many error: " << err << std::endl;
              return false;
            }
        }
        if(document_index < 1) {
          std::cerr << "ondemand should have accessed at least one document" << std::endl;
          return false;
        }
        TEST_SUCCEED();
    }
    bool issue1729() {
        // This example is not a good application of iterate_many since there is
        // a single JSON document.
        TEST_START();
        auto json = R"({
	"t": 5649,
	"ng": 5,
	"lu": 4086,
	"g": [{
			"t": "Temps",
			"xvy": 0,
			"pd": 60,
			"sz": 3,
			"l": [
				"Low Temp",
				"High Temp",
				"Smooth Avg"
			],
			"c": [
				"green",
				"orange",
				"cyan"
			],
			"d": [
				80.48750305,
				80.82499694,
				80.65625
			]
		},
		{
			"t": "Pump Status",
			"xvy": 0,
			"pd": 60,
			"sz": 3,
			"l": [
				"Pump",
				"Thermal",
				"Light"
			],
			"c": [
				"green",
				"orange",
				"cyan"
			],
			"d": [
				0,
				0,
				0
			]
		},
		{
			"t": "Lux",
			"xvy": 0,
			"pd": 60,
			"sz": 4,
			"l": [
				"Value",
				"Smooth",
				"Low",
				"High"
			],
			"c": [
				"green",
				"orange",
				"cyan",
				"yellow"
			],
			"d": [
				2274.62939453,
				2277.45947265,
				4050,
				4500
			]
		},
		{
			"t": "Temp Diff",
			"xvy": 0,
			"pd": 60,
			"sz": 4,
			"l": [
				"dFarenheit",
				"sum",
				"Low",
				"High"
			],
			"c": [
				"green",
				"orange",
				"cyan",
				"yellow"
			],
			"d": [
				0,
				0,
				0.5,
				10
			]
		},
		{
			"t": "Power",
			"xvy": 0,
			"pd": 60,
			"sz": 3,
			"l": [
				"watts (est. light) ",
				"watts (1.9~gpm) ",
				"watts (1gpm) "
			],
			"c": [
				"green",
				"orange",
				"cyan"
			],
			"d": [
				181.78063964,
				114.88922882,
				59.35943603
			]
		}
	]
})"_padded;

        ondemand::parser parser;
        ondemand::document_stream stream;
        auto oderror = parser.iterate_many(json).get(stream);
        if (oderror) {
          std::cerr << "ondemand iterate_many error: " << oderror << std::endl;
          return false;
        }
        auto i = stream.begin();
        for (; i != stream.end(); ++i) {
            ondemand::document_reference doc;
            auto err = (*i).get(doc);
            if(err != SUCCESS) {
              std::cerr << "ondemand iterate_many error: " << err << std::endl;
              return false;
            }
        }
        TEST_SUCCEED();
    }

    bool issue1977() {
        TEST_START();
        std::string json = R"( 1111 })";
        ondemand::parser odparser;
        ondemand::document_stream odstream;
        ASSERT_SUCCESS(odparser.iterate_many(json).get(odstream));

        auto i = odstream.begin();
        for (; i != odstream.end(); ++i) {
            ASSERT_TRUE(false);
        }
        ASSERT_TRUE(i.current_index() == 0);
        TEST_SUCCEED();
    }

    bool issue1683() {
        TEST_START();
        std::string json = R"([1,2,3,4,5]
[1,2,3,4,5]
[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100]
[1,2,3,4,5])";

        ondemand::parser odparser;
        ondemand::document_stream odstream;

        // iterate_many all at once
        auto oderror = odparser.iterate_many(json, 50).get(odstream);
        if (oderror) { std::cerr << "ondemand iterate_many error: " << oderror << std::endl; return false; }

        size_t currindex = 0;
        auto i = odstream.begin();
        for (; i != odstream.end(); ++i) {
            ondemand::document_reference doc;
            auto err = (*i).get(doc);
            if(err == SUCCESS) { if(!process_doc(doc)) {return false; } }
            currindex = i.current_index();
            if (err == simdjson::CAPACITY) {
                ASSERT_EQUAL(currindex, 24);
                ASSERT_EQUAL(odstream.truncated_bytes(), 305);
                break;
            } else if (err) {
               TEST_FAIL(std::string("ondemand: error accessing jsonpointer: ") + simdjson::error_message(err));
            }
        }
        ASSERT_EQUAL(odstream.truncated_bytes(), 305);

        // iterate line-by-line
        std::stringstream ss(json);
        std::string oneline;
        oneline.reserve(json.size() + SIMDJSON_PADDING);
        while (getline(ss, oneline)) {
            ondemand::document doc;
            ASSERT_SUCCESS(odparser.iterate(oneline).get(doc));
            if( ! process_doc(doc) ) { return false; }
        }
        TEST_SUCCEED();
    }

    bool simple_document_iteration() {
        TEST_START();
        auto json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3])"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        std::string_view expected[4] = {"[1,[1,2]]", "{\"a\":1,\"b\":2}", "{\"o\":{\"1\":1,\"2\":2}}", "[1,2,3]"};
        size_t counter{0};
        for(auto doc : stream) {
            ASSERT_TRUE(counter < 4);
            std::string_view view;
            ASSERT_SUCCESS(to_json_string(doc).get(view));
            ASSERT_EQUAL(view, expected[counter++]);
        }
        ASSERT_EQUAL(counter, 4);
        TEST_SUCCEED();
    }

    bool simple_document_iteration_multiple_batches() {
        TEST_START();
        auto json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3])"_padded;
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
        auto json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3])"_padded;
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
        simdjson_result<ondemand::document_reference> xxx = *i;
        ASSERT_SUCCESS( xxx.find_field("a").get(x) );
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

    bool skipbom() {
        TEST_START();
        auto json = "\xEF\xBB\xBF[1,[1,2]] {\"a\":1,\"b\":2} {\"o\":{\"1\":1,\"2\":2}} [1,2,3]"_padded;
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
        simdjson_result<ondemand::document_reference> xxx = *i;
        ASSERT_SUCCESS( xxx.find_field("a").get(x) );
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
        auto json = R"({"z":5}  {"1":1,"2":2,"4":4} [7,  10,   9]  [15,  11,   12, 13]  [154,  110,   112, 1311])"_padded;
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
        ASSERT_EQUAL(stream.truncated_bytes(), json.length());
        TEST_SUCCEED();
    }

    bool large_window() {
        TEST_START();
    #if SIZE_MAX > 17179869184
        auto json = R"({"error":[],"result":{"token":"xxx"}}{"error":[],"result":{"token":"xxx"}})"_padded;
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
        auto input = R"(                               [1,1] [1,2] [1,3]  [1,4] [1,5] [1,6] [1,7] [1,8] [1,9] [1,10] [1,11] [1,12] [1,13] [1,14] [1,15] )"_padded;;
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
        auto input = R"(                                                                                                                                                           [1,1] [1,2] [1,3]  [1,4] [1,5] [1,6] [1,7] [1,8] [1,9] [1,10] [1,11] [1,12] [1,13] [1,14] [1,15] )"_padded;;
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
        auto json = R"({"f[)"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        size_t count{0};
        for (auto doc : stream) {
            (void)doc;
            count++;
        }
        ASSERT_EQUAL(count,0);
        TEST_SUCCEED();
    }

    bool adversarial_single_document_array() {
        TEST_START();
        auto json = R"(["this is an unclosed string ])"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        size_t count{0};
        for (auto doc : stream) {
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
            for (auto doc : stream) {
                int64_t keyid{};
                ASSERT_SUCCESS( doc["id"].get(keyid) );
                ASSERT_EQUAL( keyid, int64_t(count) );

                count++;
            }
            ASSERT_EQUAL(count,n_records);
        }
        TEST_SUCCEED();
    }


    bool issue1668() {
        TEST_START();
        auto json = R"([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100])"_padded;

        ondemand::parser odparser;
        ondemand::document_stream odstream;
        ASSERT_SUCCESS( odparser.iterate_many(json.data(), json.length(), 50).get(odstream) );
        for (auto doc: odstream) {
            ondemand::value val;
            ASSERT_ERROR(doc.at_pointer("/40").get(val), CAPACITY);
            ASSERT_EQUAL(odstream.truncated_bytes(), json.length());
        }
        TEST_SUCCEED();
    }

    bool issue1668_long() {
        TEST_START();
        auto json = R"([1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100])"_padded;

        ondemand::parser odparser;
        ondemand::document_stream odstream;
        size_t counter{0};
        ASSERT_SUCCESS( odparser.iterate_many(json.data(), json.length(), 50).get(odstream) );
        for (auto doc: odstream) {
            if(counter < 6) {
                int64_t val{};
                ASSERT_SUCCESS(doc.at_pointer("/4").get(val));
                ASSERT_EQUAL(val, 5);
            } else {
                ondemand::value val;
                ASSERT_ERROR(doc.at_pointer("/4").get(val), CAPACITY);
                // We left 293 bytes unprocessed.
                ASSERT_EQUAL(odstream.truncated_bytes(), 293);
            }
            counter++;
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
            for (auto doc : stream) {
                int64_t keyid{};
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
        auto input = R"([1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] )"_padded;;
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
        auto input = R"([1,23] [1,23] [1,23] [1,23 [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] )"_padded;
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

    bool fuzzaccess() {
        TEST_START();
        // Issue 38801 in oss-fuzz
        auto json = "\xff         \n~~\n{}"_padded;
        ondemand::parser parser;
        ondemand::document_stream docs;
        ASSERT_SUCCESS(parser.iterate_many(json).get(docs));
        size_t bool_count = 0;
        size_t total_count = 0;
        for (auto doc : docs) {
          total_count++;
          bool b;
          if(doc.get_bool().get(b) == SUCCESS) { bool_count +=b; }
          // If we don't access the document at all, other than get_bool(),
          // then some simdjson kernels will allow you to iterate through
          // 3 'documents'. By asking for the type, we make the iteration
          // terminate after the first 'document'.
          ondemand::json_type t;
          if(doc.type().get(t) != SUCCESS) { break; }
        }
        return (bool_count == 0) && (total_count == 1);
    }

    bool string_with_trailing() {
        TEST_START();
        auto json = R"( "a string" badstuff )"_padded;
        ondemand::parser parser;
        ondemand::document_stream doc;
        ASSERT_SUCCESS(parser.iterate_many(json).get(doc));
        std::string_view view;
        ASSERT_SUCCESS((*doc.begin()).get_string().get(view));
        ASSERT_EQUAL(view,"a string");
        TEST_SUCCEED();
    }

    bool uint64_with_trailing() {
        TEST_START();
        auto json = R"( 133 badstuff )"_padded;
        ondemand::parser parser;
        ondemand::document_stream doc;
        ASSERT_SUCCESS(parser.iterate_many(json).get(doc));
        uint64_t x;
        ASSERT_SUCCESS((*doc.begin()).get_uint64().get(x));
        ASSERT_EQUAL(x,133);
        TEST_SUCCEED();
    }

    bool int64_with_trailing() {
        TEST_START();
        auto json = R"( 133 badstuff )"_padded;
        ondemand::parser parser;
        ondemand::document_stream doc;
        ASSERT_SUCCESS(parser.iterate_many(json).get(doc));
        int64_t x;
        ASSERT_SUCCESS((*doc.begin()).get_int64().get(x));
        ASSERT_EQUAL(x,133);
        TEST_SUCCEED();
    }

    bool double_with_trailing() {
        TEST_START();
        auto json = R"( 133 badstuff )"_padded;
        ondemand::parser parser;
        ondemand::document_stream doc;
        ASSERT_SUCCESS(parser.iterate_many(json).get(doc));
        double x;
        ASSERT_SUCCESS((*doc.begin()).get_double().get(x));
        ASSERT_EQUAL(x,133);
        TEST_SUCCEED();
    }


    bool bool_with_trailing() {
        TEST_START();
        auto json = R"( true badstuff )"_padded;
        ondemand::parser parser;
        ondemand::document_stream doc;
        ASSERT_SUCCESS(parser.iterate_many(json).get(doc));
        bool x;
        ASSERT_SUCCESS((*doc.begin()).get_bool().get(x));
        ASSERT_EQUAL(x,true);
        TEST_SUCCEED();
    }

    bool null_with_trailing() {
        TEST_START();
        auto json = R"( null badstuff )"_padded;
        ondemand::parser parser;
        ondemand::document_stream doc;
        ASSERT_SUCCESS(parser.iterate_many(json).get(doc));
        bool n;
        ASSERT_SUCCESS((*doc.begin()).is_null().get(n));
        ASSERT_EQUAL(n,true);
        TEST_SUCCEED();
    }

    bool run() {
        return
            skipbom() &&
            issue1977() &&
            string_with_trailing() &&
            uint64_with_trailing() &&
            int64_with_trailing() &&
            bool_with_trailing() &&
            null_with_trailing() &&
            truncated_utf8() &&
            issue1729() &&
            fuzzaccess() &&
            issue1683() &&
            issue1668() &&
            issue1668_long() &&
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