#include <string>
using namespace std::string_literals;

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
                std::string_view document_string;
                err = doc.get_string().get(document_string);
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

    bool issue2170() {
        TEST_START();
        auto json = R"(1 2 3)"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        auto i = stream.begin();
        size_t count{0};
        std::vector<size_t> indexes = { 0, 2, 4 };
        std::vector<std::string_view> expected = { "1", "2", "3" };

        for(; i != stream.end(); ++i) {
            ASSERT_SUCCESS(i.error());
            ASSERT_TRUE(count < 3);
            ASSERT_EQUAL(i.current_index(), indexes[count]);
            ASSERT_EQUAL(i.source(), expected[count]);
            count++;
        }
        TEST_SUCCEED();
    }

    bool issue2181() {
        TEST_START();
        auto json = R"(1 2 34)"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        auto i = stream.begin();
        size_t count{0};
        std::vector<size_t> indexes = { 0, 2, 4 };
        std::vector<std::string_view> expected = { "1", "2", "34" };

        for(; i != stream.end(); ++i) {
            ASSERT_SUCCESS(i.error());
            ASSERT_TRUE(count < 3);
            ASSERT_EQUAL(i.current_index(), indexes[count]);
            ASSERT_EQUAL(i.source(), expected[count]);
            count++;
        }
        TEST_SUCCEED();
    }

    bool issue_non_ascii_separator_source() {
        TEST_START();
        std::string bytes = "1 ";
        bytes.push_back(char(0xFF));
        bytes += " 2";
        simdjson::padded_string json(bytes);

        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));

        auto i = stream.begin();
        ASSERT_TRUE(i != stream.end());
        std::string_view source = i.source();
        ASSERT_TRUE(!source.empty());
        ASSERT_EQUAL(source.front(), '1');
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
               TEST_FAIL("ondemand: error accessing jsonpointer: "s + simdjson::error_message(err));
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

    bool simple_document_iteration_advance() {
        TEST_START();
        auto json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3])"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        std::string_view expected[4] = {"[1,[1,2]]", "{\"a\":1,\"b\":2}", "{\"o\":{\"1\":1,\"2\":2}}", "[1,2,3]"};
        auto iter = stream.begin();
        for (auto i = 0; i < 4; i++) {
            auto doc = *iter;
            std::string_view view;
            ASSERT_SUCCESS(to_json_string(doc).get(view));
            ASSERT_EQUAL(view, expected[i]);
            std::advance(iter, 1);
        }
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


    bool issue2137() {
        TEST_START();
        auto json = "true  {  "_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        for (auto doc: stream) {
            bool val{};
            ASSERT_SUCCESS(doc.get_bool().get(val));
            std::string_view raw_json;
            ASSERT_SUCCESS(doc.raw_json_token().get(raw_json));
            ASSERT_EQUAL(val, 1);
            std::string s(raw_json);
            ASSERT_EQUAL(s, "true  ");
        }
        size_t t = stream.truncated_bytes();
        ASSERT_EQUAL(3, t);
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
          // 3 'documents'. By trying to access the content, we make the iteration
          // terminate after the first 'document'.
          std::string_view document_string;
          if(doc.get_string().get(document_string) != SUCCESS) { break; }
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

    bool uint32_with_trailing() {
        TEST_START();
        auto json = R"( 133 badstuff )"_padded;
        ondemand::parser parser;
        ondemand::document_stream doc;
        ASSERT_SUCCESS(parser.iterate_many(json).get(doc));
        uint32_t x;
        ASSERT_SUCCESS((*doc.begin()).get_uint32().get(x));
        ASSERT_EQUAL(x,133);
        TEST_SUCCEED();
    }

    bool int32_with_trailing() {
        TEST_START();
        auto json = R"( 133 badstuff )"_padded;
        ondemand::parser parser;
        ondemand::document_stream doc;
        ASSERT_SUCCESS(parser.iterate_many(json).get(doc));
        int32_t x;
        ASSERT_SUCCESS((*doc.begin()).get_int32().get(x));
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

    bool json_sequence_tests() {
        TEST_START();
        ondemand::parser parser;
        ondemand::document_stream stream;

        // Verify enum values
        static_assert(static_cast<int>(simdjson::stream_format::whitespace_delimited) == 0, "whitespace_delimited should be 0");
        static_assert(static_cast<int>(simdjson::stream_format::json_sequence) == 1, "json_sequence should be 1");

        SUBTEST("whitespace_delimited format works", ([&]() {
            auto input = R"({"a":1} {"b":2} {"c":3})"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::whitespace_delimited).get(stream));
            size_t count = 0;
            for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
            ASSERT_EQUAL(count, size_t(3));
            return true;
        }()));

        SUBTEST("basic RS-delimited objects with LF", ([&]() {
            auto input = "\x1e{\"a\":1}\n\x1e{\"b\":2}\n\x1e{\"c\":3}\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            const char* keys[] = {"a", "b", "c"};
            int64_t expected[] = {1, 2, 3};
            size_t count = 0;
            for (auto doc : stream) {
                ASSERT_SUCCESS(doc.error());
                int64_t value;
                ASSERT_SUCCESS(doc[keys[count]].get(value));
                ASSERT_EQUAL(value, expected[count]);
                count++;
            }
            ASSERT_EQUAL(count, size_t(3));
            return true;
        }()));

        SUBTEST("multiple documents without trailing LF", ([&]() {
            auto input = "\x1e{\"a\":1}\x1e{\"b\":2}\x1e{\"c\":3}"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            size_t count = 0;
            for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
            ASSERT_EQUAL(count, size_t(3));
            return true;
        }()));

        SUBTEST("single document without LF", ([&]() {
            auto input = "\x1e{\"a\":1}"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            size_t count = 0;
            for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
            ASSERT_EQUAL(count, size_t(1));
            return true;
        }()));

        SUBTEST("scalar documents", ([&]() {
            auto input = "\x1e" "42\n" "\x1e" "\"hello\"\n" "\x1e" "true\n" "\x1e" "null\n" "\x1e" "[1,2,3]\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            size_t count = 0;
            for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
            ASSERT_EQUAL(count, size_t(5));
            return true;
        }()));

        SUBTEST("current_index points to JSON value not RS", ([&]() {
            // Layout: RS(0) 1(1) LF(2) RS(3) 2(4) LF(5)
            auto input = "\x1e" "1\n" "\x1e" "2\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            auto it = stream.begin();
            ASSERT_EQUAL(it.current_index(), size_t(1));  // "1" at position 1
            ++it;
            ASSERT_EQUAL(it.current_index(), size_t(4));  // "2" at position 4
            return true;
        }()));

        SUBTEST("RS-only input produces 0 documents", ([&]() {
            auto input = "\x1e\n\x1e\n"_padded;
            auto result = parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream);
            // We accept either EMPTY error or successful iteration with 0 documents
            if (result == simdjson::SUCCESS) {
                size_t count = 0;
                for (auto doc : stream) { (void)doc; count++; }
                ASSERT_EQUAL(count, size_t(0));
            }
            // EMPTY error is also acceptable
            return true;
        }()));

        SUBTEST("RS-only input variants accept EMPTY or zero documents", ([&]() {
            // Several RS-only patterns. For each, iterate_many must either:
            //   (a) succeed and yield zero documents on iteration, or
            //   (b) report simdjson::EMPTY.
            // Any other outcome (including a non-EMPTY error, or producing
            // ghost documents) is a failure.
            const std::string inputs[] = {
                "\x1e"s,                  // single RS, no LF
                "\x1e\n"s,                // RS + LF
                "\x1e\x1e"s,              // back-to-back RS
                "\x1e \t\n"s,             // RS + whitespace
                "\x1e\n\x1e\n\x1e\n"s,    // three RSes, no payloads
            };
            for (const auto& s : inputs) {
                auto input = simdjson::padded_string(s);
                auto result = parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream);
                if (result == simdjson::SUCCESS) {
                    size_t count = 0;
                    for (auto doc : stream) { (void)doc; count++; }
                    ASSERT_EQUAL(count, size_t(0));
                } else {
                    ASSERT_EQUAL(result, simdjson::EMPTY);
                }
            }
            return true;
        }()));

        SUBTEST("string documents current_index and source", ([&]() {
            // Layout: RS(0) "(1) h(2) e(3) l(4) l(5) o(6) "(7) LF(8)
            //         RS(9) "(10) w(11) o(12) r(13) l(14) d(15) "(16) LF(17)
            // Regression test: when RS is followed by a JSON string, the
            // first structural after the RS is the opening quote. The stream
            // must report the document position at the opening quote (not the
            // RS) and source() must yield the quoted string verbatim.
            auto input = "\x1e\"hello\"\n\x1e\"world\"\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            auto it = stream.begin();
            ASSERT_EQUAL(it.current_index(), size_t(1));
            ASSERT_EQUAL(it.source(), std::string_view("\"hello\""));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                std::string_view sv;
                ASSERT_SUCCESS(doc.get_string().get(sv));
                ASSERT_EQUAL(sv, std::string_view("hello"));
            }
            ++it;
            ASSERT_EQUAL(it.current_index(), size_t(10));
            ASSERT_EQUAL(it.source(), std::string_view("\"world\""));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                std::string_view sv;
                ASSERT_SUCCESS(doc.get_string().get(sv));
                ASSERT_EQUAL(sv, std::string_view("world"));
            }
            return true;
        }()));

        SUBTEST("whitespace between RS and value", ([&]() {
            auto input = "\x1e  {\"a\":1}\n\x1e\t\n{\"b\":2}\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            size_t count = 0;
            for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
            ASSERT_EQUAL(count, size_t(2));
            return true;
        }()));

        SUBTEST("mixed document types in sequence", ([&]() {
            auto input = "\x1e" "123\n" "\x1e" "{\"x\":1}\n" "\x1e" "[1,2]\n" "\x1e" "\"str\"\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            auto it = stream.begin();
            // Doc 0: number
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                int64_t num; ASSERT_SUCCESS(doc.get_int64().get(num)); ASSERT_EQUAL(num, int64_t(123));
            }
            ++it;
            // Doc 1: object
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                int64_t x; ASSERT_SUCCESS(doc["x"].get(x)); ASSERT_EQUAL(x, int64_t(1));
            }
            ++it;
            // Doc 2: array
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                ondemand::array arr; ASSERT_SUCCESS(doc.get_array().get(arr));
            }
            ++it;
            // Doc 3: string
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                std::string_view sv; ASSERT_SUCCESS(doc.get_string().get(sv)); ASSERT_EQUAL(sv, std::string_view("str"));
            }
            return true;
        }()));

        SUBTEST("source returns correct document slice", ([&]() {
            auto input = "\x1e" "[1,2,3]\n" "\x1e" "{\"a\":1}\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            auto it = stream.begin();
            ASSERT_EQUAL(it.source(), std::string_view("[1,2,3]"));
            ++it;
            ASSERT_EQUAL(it.source(), std::string_view("{\"a\":1}"));
            return true;
        }()));

        SUBTEST("multibatch current_index and source", ([&]() {
            auto input = "\x1e {\"a\":1}\n\x1e\t{\"b\":2}\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, 10, simdjson::stream_format::json_sequence).get(stream));
            auto it = stream.begin();
            ASSERT_EQUAL(it.current_index(), size_t(2));
            ASSERT_EQUAL(it.source(), std::string_view("{\"a\":1}"));
            ++it;
            ASSERT_EQUAL(it.current_index(), size_t(12));
            ASSERT_EQUAL(it.source(), std::string_view("{\"b\":2}"));
            return true;
        }()));

        SUBTEST("small batch reports capacity", ([&]() {
            std::string json_sequence_input = "\x1e[";
            for (size_t i = 0; i < 2000; i++) {
                json_sequence_input += '1';
                json_sequence_input += (i + 1 < 2000 ? ',' : ']');
            }
            json_sequence_input += '\n';
            auto input = simdjson::padded_string(json_sequence_input);
            ASSERT_SUCCESS(parser.iterate_many(input, 1024, simdjson::stream_format::json_sequence).get(stream));
            auto it = stream.begin();
            ASSERT_ERROR(it.error(), CAPACITY);
            return true;
        }()));

        SUBTEST("scalar trio: number, true, string", ([&]() {
            // Layout: RS(0) 1(1) LF(2) RS(3) t(4) r(5) u(6) e(7) LF(8)
            //         RS(9) "(10) x(11) "(12) LF(13)
            auto input = "\x1e" "1\n" "\x1e" "true\n" "\x1e" "\"x\"\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            auto it = stream.begin();

            // doc 0: number 1
            ASSERT_EQUAL(it.current_index(), size_t(1));
            ASSERT_EQUAL(it.source(), std::string_view("1"));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, int64_t(1));
            }
            ++it;

            // doc 1: true
            ASSERT_EQUAL(it.current_index(), size_t(4));
            ASSERT_EQUAL(it.source(), std::string_view("true"));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                bool v; ASSERT_SUCCESS(doc.get_bool().get(v));
                ASSERT_TRUE(v);
            }
            ++it;

            // doc 2: "x"
            ASSERT_EQUAL(it.current_index(), size_t(10));
            ASSERT_EQUAL(it.source(), std::string_view("\"x\""));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                std::string_view sv; ASSERT_SUCCESS(doc.get_string().get(sv));
                ASSERT_EQUAL(sv, std::string_view("x"));
            }
            return true;
        }()));

        SUBTEST("scalar trio multibatch (small batch_size)", ([&]() {
            // Same input as the scalar trio test, but batch_size=6 forces
            // the stream through multiple stage1 invocations. Behavior must
            // match the single-batch case for current_index and source.
            auto input = "\x1e" "1\n" "\x1e" "true\n" "\x1e" "\"x\"\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, 6, simdjson::stream_format::json_sequence).get(stream));
            auto it = stream.begin();

            ASSERT_EQUAL(it.current_index(), size_t(1));
            ASSERT_EQUAL(it.source(), std::string_view("1"));
            ++it;
            ASSERT_EQUAL(it.current_index(), size_t(4));
            ASSERT_EQUAL(it.source(), std::string_view("true"));
            ++it;
            ASSERT_EQUAL(it.current_index(), size_t(10));
            ASSERT_EQUAL(it.source(), std::string_view("\"x\""));
            return true;
        }()));

        SUBTEST("large synthetic scalar stream multibatch", ([&]() {
            // Stress the windowing / multi-stage1 path with 256 RS-prefixed
            // scalar documents rotating through all four scalar types. A
            // small batch_size (64) forces ~25 stage1 invocations over
            // ~1.5 KB of input. Every document must parse and appear in
            // order.
            constexpr size_t N = 256;
            std::string bytes;
            for (size_t i = 0; i < N; i++) {
                bytes += '\x1e';
                switch (i % 4) {
                    case 0: bytes += std::to_string(i); break;
                    case 1: bytes += (i & 1) ? "true" : "false"; break;
                    case 2: bytes += "\"s" + std::to_string(i) + "\""; break;
                    case 3: bytes += "null"; break;
                }
                bytes += '\n';
            }
            auto input = simdjson::padded_string(bytes);
            ASSERT_SUCCESS(parser.iterate_many(input, 64, simdjson::stream_format::json_sequence).get(stream));
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < N);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                switch (i % 4) {
                    case 0: {
                        int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                        ASSERT_EQUAL(v, int64_t(i));
                        break;
                    }
                    case 1: {
                        bool v; ASSERT_SUCCESS(doc.get_bool().get(v));
                        ASSERT_EQUAL(v, bool(i & 1));
                        break;
                    }
                    case 2: {
                        std::string_view v; ASSERT_SUCCESS(doc.get_string().get(v));
                        std::string want = "s" + std::to_string(i);
                        ASSERT_EQUAL(v, std::string_view(want));
                        break;
                    }
                    case 3: {
                        bool is_null;
                        ASSERT_SUCCESS(doc.is_null().get(is_null));
                        ASSERT_TRUE(is_null);
                        break;
                    }
                }
                i++;
            }
            ASSERT_EQUAL(i, N);
            return true;
        }()));

        SUBTEST("large synthetic scalar stream multibatch with separator noise", ([&]() {
            // Same as the large stress test, but with extra RSes sprinkled
            // every 7 documents (empty leading record within a valid
            // record sequence) plus a trailing bare RS. Strict
            // assertions: every document must come through and parse
            // cleanly, with the correct typed value.
            constexpr size_t N = 256;
            std::string bytes;
            for (size_t i = 0; i < N; i++) {
                bytes += '\x1e';
                if (i % 7 == 0 && i > 0) { bytes += '\x1e'; }  // empty record
                switch (i % 4) {
                    case 0: bytes += std::to_string(i); break;
                    case 1: bytes += (i & 1) ? "true" : "false"; break;
                    case 2: bytes += "\"s" + std::to_string(i) + "\""; break;
                    case 3: bytes += "null"; break;
                }
                bytes += '\n';
            }
            bytes += '\x1e';  // trailing bare RS
            auto input = simdjson::padded_string(bytes);
            ASSERT_SUCCESS(parser.iterate_many(input, 64, simdjson::stream_format::json_sequence).get(stream));
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < N);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                switch (i % 4) {
                    case 0: {
                        int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                        ASSERT_EQUAL(v, int64_t(i));
                        break;
                    }
                    case 1: {
                        bool v; ASSERT_SUCCESS(doc.get_bool().get(v));
                        ASSERT_EQUAL(v, bool(i & 1));
                        break;
                    }
                    case 2: {
                        std::string_view v; ASSERT_SUCCESS(doc.get_string().get(v));
                        std::string want = "s" + std::to_string(i);
                        ASSERT_EQUAL(v, std::string_view(want));
                        break;
                    }
                    case 3: {
                        bool is_null;
                        ASSERT_SUCCESS(doc.is_null().get(is_null));
                        ASSERT_TRUE(is_null);
                        break;
                    }
                }
                i++;
            }
            ASSERT_EQUAL(i, N);
            return true;
        }()));

        // -------- RS spacing variants --------
        // The scanner classifies 0x1E as a scalar character, so the way an
        // RS is positioned relative to surrounding whitespace affects what
        // structural_indexes look like after stage1. These tests lock down
        // every reasonable arrangement: RS followed by the canonical LF,
        // by CRLF, by space, by tab, by mixed whitespace, with whitespace
        // on both sides, and tight (no surrounding whitespace at all)
        // around containers and strings.

        SUBTEST("RS followed by LF then value (canonical RFC)", ([&]() {
            // bytes: 1e 31 0a 1e 32 0a 1e 33 0a
            auto input = "\x1e" "1\n" "\x1e" "2\n" "\x1e" "3\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            int64_t expected[3] = {1, 2, 3};
            size_t expected_idx[3] = {1, 4, 7};
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < 3);
                ASSERT_EQUAL(it.current_index(), expected_idx[i]);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, expected[i]);
                i++;
            }
            ASSERT_EQUAL(i, size_t(3));
            return true;
        }()));

        SUBTEST("RS followed by CRLF then value", ([&]() {
            // bytes: 1e 31 0d 0a 1e 32 0d 0a 1e 33 0d 0a
            auto input = "\x1e" "1\r\n" "\x1e" "2\r\n" "\x1e" "3\r\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            int64_t expected[3] = {1, 2, 3};
            size_t expected_idx[3] = {1, 5, 9};
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < 3);
                ASSERT_EQUAL(it.current_index(), expected_idx[i]);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, expected[i]);
                i++;
            }
            ASSERT_EQUAL(i, size_t(3));
            return true;
        }()));

        SUBTEST("RS followed by single space then value", ([&]() {
            // bytes: 1e 20 31 0a 1e 20 32 0a 1e 20 33 0a
            auto input = "\x1e 1\n\x1e 2\n\x1e 3\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            int64_t expected[3] = {1, 2, 3};
            size_t expected_idx[3] = {2, 6, 10};
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < 3);
                ASSERT_EQUAL(it.current_index(), expected_idx[i]);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, expected[i]);
                i++;
            }
            ASSERT_EQUAL(i, size_t(3));
            return true;
        }()));

        SUBTEST("RS followed by tab then value", ([&]() {
            // bytes: 1e 09 31 0a 1e 09 32 0a 1e 09 33 0a
            auto input = "\x1e\t1\n\x1e\t2\n\x1e\t3\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            int64_t expected[3] = {1, 2, 3};
            size_t expected_idx[3] = {2, 6, 10};
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < 3);
                ASSERT_EQUAL(it.current_index(), expected_idx[i]);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, expected[i]);
                i++;
            }
            ASSERT_EQUAL(i, size_t(3));
            return true;
        }()));

        SUBTEST("RS followed by mixed whitespace then value", ([&]() {
            // All four JSON whitespace characters between the RS and the
            // value. bytes: 1e 20 09 0d 0a 31 0a 1e 20 09 0d 0a 32 0a
            auto input = "\x1e \t\r\n1\n\x1e \t\r\n2\n"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            int64_t expected[2] = {1, 2};
            size_t expected_idx[2] = {5, 12};
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < 2);
                ASSERT_EQUAL(it.current_index(), expected_idx[i]);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, expected[i]);
                i++;
            }
            ASSERT_EQUAL(i, size_t(2));
            return true;
        }()));

        SUBTEST("RS with whitespace on both sides of scalar value", ([&]() {
            // Whitespace after the RS AND between the value and the next RS.
            // bytes: 1e 20 31 20 1e 20 32 20 1e 20 33
            auto input = "\x1e 1 \x1e 2 \x1e 3"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            int64_t expected[3] = {1, 2, 3};
            size_t expected_idx[3] = {2, 6, 10};
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < 3);
                ASSERT_EQUAL(it.current_index(), expected_idx[i]);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, expected[i]);
                i++;
            }
            ASSERT_EQUAL(i, size_t(3));
            return true;
        }()));

        SUBTEST("tight RS around arrays (no whitespace anywhere)", ([&]() {
            // No whitespace between RS and value, between values, or
            // anywhere else. Arrays survive because their closing ']' is a
            // structural character that naturally terminates the preceding
            // scalar run.
            // bytes: 1e 5b 31 2c 32 5d 1e 5b 33 2c 34 5d 1e 5b 35 2c 36 5d
            auto input = "\x1e[1,2]\x1e[3,4]\x1e[5,6]"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            size_t expected_idx[3] = {1, 7, 13};
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < 3);
                ASSERT_EQUAL(it.current_index(), expected_idx[i]);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                ondemand::array arr;
                ASSERT_SUCCESS(doc.get_array().get(arr));
                size_t n = 0;
                for (auto e : arr) { (void)e; n++; }
                ASSERT_EQUAL(n, size_t(2));
                i++;
            }
            ASSERT_EQUAL(i, size_t(3));
            return true;
        }()));

        SUBTEST("tight RS around string scalars (no whitespace anywhere)", ([&]() {
            // bytes: 1e 22 61 22 1e 22 62 22 1e 22 63 22
            auto input = "\x1e\"a\"\x1e\"b\"\x1e\"c\""_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            const char* expected[3] = {"a", "b", "c"};
            size_t expected_idx[3] = {1, 5, 9};
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < 3);
                ASSERT_EQUAL(it.current_index(), expected_idx[i]);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                std::string_view sv;
                ASSERT_SUCCESS(doc.get_string().get(sv));
                ASSERT_EQUAL(sv, std::string_view(expected[i]));
                i++;
            }
            ASSERT_EQUAL(i, size_t(3));
            return true;
        }()));

        SUBTEST("tight RS around mixed containers (no whitespace anywhere)", ([&]() {
            // Object, array, object - all back-to-back with only RS between.
            auto input = "\x1e{\"a\":1}\x1e[1,2]\x1e{\"b\":2}"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            auto it = stream.begin();

            // doc 0: {"a":1}
            ASSERT_EQUAL(it.current_index(), size_t(1));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                int64_t a; ASSERT_SUCCESS(doc["a"].get(a));
                ASSERT_EQUAL(a, int64_t(1));
            }
            ++it;

            // doc 1: [1,2]
            ASSERT_EQUAL(it.current_index(), size_t(9));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                ondemand::array arr;
                ASSERT_SUCCESS(doc.get_array().get(arr));
                size_t arr_count = 0;
                for (auto e : arr) { (void)e; arr_count++; }
                ASSERT_EQUAL(arr_count, size_t(2));
            }
            ++it;

            // doc 2: {"b":2}
            ASSERT_EQUAL(it.current_index(), size_t(15));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                int64_t b; ASSERT_SUCCESS(doc["b"].get(b));
                ASSERT_EQUAL(b, int64_t(2));
            }
            return true;
        }()));

        SUBTEST("tight RS multibatch: 100 objects no whitespace, bs=32", ([&]() {
            // 100 tight objects `\x1e{"i":0}\x1e{"i":1}...` with a small
            // batch size to stress the multibatch path. Objects are
            // delimited by their closing '}' so this case works without
            // any whitespace.
            constexpr size_t N = 100;
            std::string bytes;
            for (size_t i = 0; i < N; i++) {
                bytes += '\x1e';
                bytes += "{\"i\":";
                bytes += std::to_string(i);
                bytes += "}";
            }
            auto input = simdjson::padded_string(bytes);
            ASSERT_SUCCESS(parser.iterate_many(input, 32, simdjson::stream_format::json_sequence).get(stream));
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < N);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v;
                ASSERT_SUCCESS(doc["i"].get(v));
                ASSERT_EQUAL(v, int64_t(i));
                i++;
            }
            ASSERT_EQUAL(i, N);
            return true;
        }()));

        SUBTEST("spaced RS multibatch: 100 int scalars RS+space+value+LF, bs=32", ([&]() {
            // 100 int scalars shaped as `\x1e <n>\n` each. Small batch
            // size (32) forces ~16 stage1 invocations over the ~500 byte
            // input.
            constexpr size_t N = 100;
            std::string bytes;
            for (size_t i = 0; i < N; i++) {
                bytes += '\x1e';
                bytes += ' ';
                bytes += std::to_string(i);
                bytes += '\n';
            }
            auto input = simdjson::padded_string(bytes);
            ASSERT_SUCCESS(parser.iterate_many(input, 32, simdjson::stream_format::json_sequence).get(stream));
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < N);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v;
                ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, int64_t(i));
                i++;
            }
            ASSERT_EQUAL(i, N);
            return true;
        }()));

        SUBTEST("leading/trailing/repeated RS separators around scalars", ([&]() {
            // RFC 7464 says each JSON text is preceded by exactly one RS.
            // The sensible interpretation of degenerate inputs:
            //   - leading RS-RS (empty record before the first scalar): skip
            //   - repeated RSes between two scalars (empty intermediate
            //     record): skip the empty record, both scalars reported
            //   - trailing bare RS with no payload: skip, no phantom doc
            // Layout: RS(0) RS(1) 1(2) LF(3) RS(4) RS(5) RS(6) 2(7) LF(8) RS(9)
            auto input = "\x1e\x1e" "1\n" "\x1e\x1e\x1e" "2\n" "\x1e"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
            int64_t expected[] = {1, 2};
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < 2);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, expected[i]);
                i++;
            }
            ASSERT_EQUAL(i, size_t(2));
            return true;
        }()));

        TEST_SUCCEED();
    }

    bool comma_delimited_tests() {
        TEST_START();
        ondemand::parser parser;
        ondemand::document_stream stream;

        SUBTEST("basic comma-separated objects", ([&]() {
            auto input = R"({"a":1},{"b":2},{"c":3})"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
            const char* keys[] = {"a", "b", "c"};
            int64_t expected[] = {1, 2, 3};
            size_t count = 0;
            for (auto doc : stream) {
                ASSERT_SUCCESS(doc.error());
                int64_t value;
                ASSERT_SUCCESS(doc[keys[count]].get(value));
                ASSERT_EQUAL(value, expected[count]);
                count++;
            }
            ASSERT_EQUAL(count, size_t(3));
            return true;
        }()));

        SUBTEST("comma-separated with whitespace", ([&]() {
            auto input = R"({"a":1} , {"b":2} , {"c":3})"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
            size_t count = 0;
            for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
            ASSERT_EQUAL(count, size_t(3));
            return true;
        }()));

        SUBTEST("comma-separated arrays", ([&]() {
            auto input = R"([1,2],[3,4],[5,6])"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
            size_t count = 0;
            for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
            ASSERT_EQUAL(count, size_t(3));
            return true;
        }()));

        SUBTEST("mixed document types", ([&]() {
            auto input = R"({"a":1},[2,3],"string",42,true,null)"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
            size_t count = 0;
            for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
            ASSERT_EQUAL(count, size_t(6));
            return true;
        }()));

        SUBTEST("nested commas preserved", ([&]() {
            auto input = R"({"a":[1,2,3]},{"b":{"c":4,"d":5}})"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
            auto it = stream.begin();
            // First doc: {"a":[1,2,3]}
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                ondemand::array arr;
                ASSERT_SUCCESS(doc["a"].get_array().get(arr));
                size_t arr_count = 0;
                for (auto elem : arr) { (void)elem; arr_count++; }
                ASSERT_EQUAL(arr_count, size_t(3));
            }
            ++it;
            // Second doc: {"b":{"c":4,"d":5}}
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                ondemand::object b_obj;
                ASSERT_SUCCESS(doc["b"].get_object().get(b_obj));
                int64_t c_val, d_val;
                ASSERT_SUCCESS(b_obj["c"].get(c_val));
                ASSERT_SUCCESS(b_obj["d"].get(d_val));
                ASSERT_EQUAL(c_val, int64_t(4));
                ASSERT_EQUAL(d_val, int64_t(5));
            }
            return true;
        }()));

        SUBTEST("single document no comma", ([&]() {
            auto input = R"({"a":1})"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
            size_t count = 0;
            for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
            ASSERT_EQUAL(count, size_t(1));
            return true;
        }()));

        SUBTEST("current_index points to JSON value", ([&]() {
            // Layout: {(0) "(1) a(2) "(3) :(4) 1(5) }(6) ,(7) {(8) ...
            auto input = R"({"a":1},{"b":2})"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
            auto it = stream.begin();
            ASSERT_EQUAL(it.current_index(), size_t(0));  // First { at position 0
            ++it;
            ASSERT_EQUAL(it.current_index(), size_t(8));  // Second { at position 8
            return true;
        }()));

        SUBTEST("source returns correct document slice", ([&]() {
            auto input = R"([1,2,3],{"a":1})"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
            auto it = stream.begin();
            ASSERT_EQUAL(it.source(), std::string_view("[1,2,3]"));
            ++it;
            ASSERT_EQUAL(it.source(), std::string_view("{\"a\":1}"));
            return true;
        }()));

        SUBTEST("multibatch current_index and source", ([&]() {
            auto input = R"({"a":1}, {"b":2})"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, 10, simdjson::stream_format::comma_delimited).get(stream));
            auto it = stream.begin();
            ASSERT_EQUAL(it.current_index(), size_t(0));
            ASSERT_EQUAL(it.source(), std::string_view("{\"a\":1}"));
            ++it;
            ASSERT_EQUAL(it.current_index(), size_t(9));
            ASSERT_EQUAL(it.source(), std::string_view("{\"b\":2}"));
            return true;
        }()));

        SUBTEST("small batch reports capacity", ([&]() {
            std::string comma_input = "  [";
            for (size_t i = 0; i < 2000; i++) {
                comma_input += '1';
                comma_input += (i + 1 < 2000 ? ',' : ']');
            }
            auto input = simdjson::padded_string(comma_input);
            ASSERT_SUCCESS(parser.iterate_many(input, 1024, simdjson::stream_format::comma_delimited).get(stream));
            auto it = stream.begin();
            ASSERT_ERROR(it.error(), CAPACITY);
            return true;
        }()));

        SUBTEST("scalar quintet 1,2,\"x\",true,null", ([&]() {
            // Five comma-delimited scalars: number, number, string, true, null.
            // Layout: 1(0) ,(1) 2(2) ,(3) "(4) x(5) "(6) ,(7) t(8) r(9) u(10) e(11) ,(12) n(13) u(14) l(15) l(16)
            //
            // Note on source(): the comma_delimited filter strips root-level
            // commas from structural_indexes but the bytes remain in the
            // buffer. source() for a scalar slices [current_index,
            // next_doc_index) and strips trailing whitespace; trailing
            // delimiter is stripped for consistency with json_sequence. The
            // assertions below lock down current_index() and parsed values
            // strictly, and source() returns the JSON value without trailing
            // delimiter.
            auto input = R"(1,2,"x",true,null)"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
            auto it = stream.begin();

            // doc 0: 1
            ASSERT_EQUAL(it.current_index(), size_t(0));
            ASSERT_EQUAL(it.source(), std::string_view("1"));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, int64_t(1));
            }
            ++it;

            // doc 1: 2
            ASSERT_EQUAL(it.current_index(), size_t(2));
            ASSERT_EQUAL(it.source(), std::string_view("2"));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, int64_t(2));
            }
            ++it;

            // doc 2: "x"
            ASSERT_EQUAL(it.current_index(), size_t(4));
            ASSERT_EQUAL(it.source(), std::string_view("\"x\""));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                std::string_view sv; ASSERT_SUCCESS(doc.get_string().get(sv));
                ASSERT_EQUAL(sv, std::string_view("x"));
            }
            ++it;

            // doc 3: true
            ASSERT_EQUAL(it.current_index(), size_t(8));
            ASSERT_EQUAL(it.source(), std::string_view("true"));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                bool v; ASSERT_SUCCESS(doc.get_bool().get(v));
                ASSERT_TRUE(v);
            }
            ++it;

            // doc 4: null (last doc, no trailing comma in source either way)
            ASSERT_EQUAL(it.current_index(), size_t(13));
            ASSERT_EQUAL(it.source(), std::string_view("null"));
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                bool is_null;
                ASSERT_SUCCESS(doc.is_null().get(is_null));
                ASSERT_TRUE(is_null);
            }
            return true;
        }()));

        SUBTEST("scalar quintet multibatch (small batch_size)", ([&]() {
            // Same input as the scalar quintet test, but batch_size=8 forces
            // the stream through multiple stage1 invocations. Behavior must
            // match the single-batch case for current_index.
            auto input = R"(1,2,"x",true,null)"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, 8, simdjson::stream_format::comma_delimited).get(stream));
            size_t count = 0;
            const size_t expected_idx[5] = {0, 2, 4, 8, 13};
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc = *it;
                ASSERT_SUCCESS(doc.error());
                ASSERT_TRUE(count < 5);
                ASSERT_EQUAL(it.current_index(), expected_idx[count]);
                count++;
            }
            ASSERT_EQUAL(count, size_t(5));
            return true;
        }()));

        SUBTEST("large synthetic scalar stream multibatch", ([&]() {
            // Stress the windowing / multi-stage1 path with 256 comma-
            // delimited scalar documents rotating through all four scalar
            // types. A small batch_size (64) forces ~20 stage1 invocations
            // over ~1.3 KB of input. Every document must parse and appear
            // in order.
            constexpr size_t N = 256;
            std::string bytes;
            for (size_t i = 0; i < N; i++) {
                if (i > 0) { bytes += ','; }
                switch (i % 4) {
                    case 0: bytes += std::to_string(i); break;
                    case 1: bytes += (i & 1) ? "true" : "false"; break;
                    case 2: bytes += "\"s" + std::to_string(i) + "\""; break;
                    case 3: bytes += "null"; break;
                }
            }
            auto input = simdjson::padded_string(bytes);
            ASSERT_SUCCESS(parser.iterate_many(input, 64, simdjson::stream_format::comma_delimited).get(stream));
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < N);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                switch (i % 4) {
                    case 0: {
                        int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                        ASSERT_EQUAL(v, int64_t(i));
                        break;
                    }
                    case 1: {
                        bool v; ASSERT_SUCCESS(doc.get_bool().get(v));
                        ASSERT_EQUAL(v, bool(i & 1));
                        break;
                    }
                    case 2: {
                        std::string_view v; ASSERT_SUCCESS(doc.get_string().get(v));
                        std::string want = "s" + std::to_string(i);
                        ASSERT_EQUAL(v, std::string_view(want));
                        break;
                    }
                    case 3: {
                        bool is_null;
                        ASSERT_SUCCESS(doc.is_null().get(is_null));
                        ASSERT_TRUE(is_null);
                        break;
                    }
                }
                i++;
            }
            ASSERT_EQUAL(i, N);
            return true;
        }()));

        SUBTEST("large synthetic scalar stream multibatch with separator noise", ([&]() {
            // Same as the large stress test, but with a leading comma,
            // extra commas sprinkled every 5 documents, and two trailing
            // commas. The comma_delimited filter is lenient about
            // degenerate separators, so this test is STRICT: exactly N
            // documents must still come out, in order, with correct
            // values.
            constexpr size_t N = 256;
            std::string bytes;
            bytes += ',';  // leading comma
            for (size_t i = 0; i < N; i++) {
                if (i > 0) { bytes += ','; }
                if (i % 5 == 0 && i > 0) { bytes += ','; }  // extra comma
                switch (i % 4) {
                    case 0: bytes += std::to_string(i); break;
                    case 1: bytes += (i & 1) ? "true" : "false"; break;
                    case 2: bytes += "\"s" + std::to_string(i) + "\""; break;
                    case 3: bytes += "null"; break;
                }
            }
            bytes += ",,";  // trailing commas
            auto input = simdjson::padded_string(bytes);
            ASSERT_SUCCESS(parser.iterate_many(input, 64, simdjson::stream_format::comma_delimited).get(stream));
            size_t i = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(i < N);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                switch (i % 4) {
                    case 0: {
                        int64_t v; ASSERT_SUCCESS(doc.get_int64().get(v));
                        ASSERT_EQUAL(v, int64_t(i));
                        break;
                    }
                    case 1: {
                        bool v; ASSERT_SUCCESS(doc.get_bool().get(v));
                        ASSERT_EQUAL(v, bool(i & 1));
                        break;
                    }
                    case 2: {
                        std::string_view v; ASSERT_SUCCESS(doc.get_string().get(v));
                        std::string want = "s" + std::to_string(i);
                        ASSERT_EQUAL(v, std::string_view(want));
                        break;
                    }
                    case 3: {
                        bool is_null;
                        ASSERT_SUCCESS(doc.is_null().get(is_null));
                        ASSERT_TRUE(is_null);
                        break;
                    }
                }
                i++;
            }
            ASSERT_EQUAL(i, N);
            return true;
        }()));

        SUBTEST("leading/trailing/repeated commas around scalars", ([&]() {
            // Comma-delimited filter is expected to be lenient: leading
            // commas, trailing commas, and repeated commas between scalars
            // all collapse to single separators. The three real scalars
            // (1, 2, "x") must be reported with no phantom documents.
            // Layout: ,(0) 1(1) ,(2) ,(3) 2(4) ,(5) ,(6) "(7) x(8) "(9) ,(10) ,(11)
            auto input = R"(,1,,2,,"x",,)"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
            size_t count = 0;
            int64_t i1 = -1, i2 = -1;
            std::string_view sx;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                if (count == 0) { ASSERT_SUCCESS(doc.get_int64().get(i1)); }
                else if (count == 1) { ASSERT_SUCCESS(doc.get_int64().get(i2)); }
                else if (count == 2) { ASSERT_SUCCESS(doc.get_string().get(sx)); }
                count++;
            }
            ASSERT_EQUAL(count, size_t(3));
            ASSERT_EQUAL(i1, int64_t(1));
            ASSERT_EQUAL(i2, int64_t(2));
            ASSERT_EQUAL(sx, std::string_view("x"));
            return true;
        }()));

        // -------- stream_format::comma_delimited_array --------
        // The comma_delimited_array mode strips the outer [ ] (plus any
        // surrounding JSON whitespace) and then delegates to comma_delimited.

        SUBTEST("comma_delimited_array basic [1,2,3]", ([&]() {
            auto input = R"([1,2,3])"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
            size_t count = 0;
            int64_t got[3] = {0, 0, 0};
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(count < 3);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                ASSERT_SUCCESS(doc.get_int64().get(got[count]));
                count++;
            }
            ASSERT_EQUAL(count, size_t(3));
            ASSERT_EQUAL(got[0], int64_t(1));
            ASSERT_EQUAL(got[1], int64_t(2));
            ASSERT_EQUAL(got[2], int64_t(3));
            return true;
        }()));

        SUBTEST("comma_delimited_array mixed scalar types", ([&]() {
            auto input = R"([1, "a", true, null, {"x":1}])"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
            size_t count = 0;
            for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
            ASSERT_EQUAL(count, size_t(5));
            return true;
        }()));

        SUBTEST("comma_delimited_array empty []", ([&]() {
            auto input = R"([])"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
            size_t count = 0;
            for (auto doc : stream) { (void)doc; count++; }
            ASSERT_EQUAL(count, size_t(0));
            return true;
        }()));

        SUBTEST("comma_delimited_array empty with interior whitespace [ ]", ([&]() {
            auto input = "[  \t\n]"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
            size_t count = 0;
            for (auto doc : stream) { (void)doc; count++; }
            ASSERT_EQUAL(count, size_t(0));
            return true;
        }()));

        SUBTEST("comma_delimited_array surrounding whitespace stripped", ([&]() {
            // All four JSON whitespace characters before the '[' and after
            // the ']'.
            auto input = " \t\n\r[ 1, 2, 3 ] \t\n\r"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
            size_t count = 0;
            int64_t sum = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v;
                ASSERT_SUCCESS(doc.get_int64().get(v));
                sum += v;
                count++;
            }
            ASSERT_EQUAL(count, size_t(3));
            ASSERT_EQUAL(sum, int64_t(6));
            return true;
        }()));

        SUBTEST("comma_delimited_array nested commas preserved", ([&]() {
            auto input = R"([{"a":1,"b":2},{"c":[3,4,5]}])"_padded;
            ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
            auto it = stream.begin();
            // doc 0: {"a":1,"b":2}
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                int64_t a, b;
                ASSERT_SUCCESS(doc["a"].get(a));
                ASSERT_SUCCESS(doc["b"].get(b));
                ASSERT_EQUAL(a, int64_t(1));
                ASSERT_EQUAL(b, int64_t(2));
            }
            ++it;
            // doc 1: {"c":[3,4,5]}
            {
                ondemand::document_reference doc;
                ASSERT_SUCCESS((*it).get(doc));
                ondemand::array arr;
                ASSERT_SUCCESS(doc["c"].get_array().get(arr));
                size_t arr_count = 0;
                for (auto e : arr) { (void)e; arr_count++; }
                ASSERT_EQUAL(arr_count, size_t(3));
            }
            return true;
        }()));

        SUBTEST("comma_delimited_array missing leading bracket is TAPE_ERROR", ([&]() {
            auto input = R"(1,2,3)"_padded;
            auto err = parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream);
            ASSERT_ERROR(err, simdjson::TAPE_ERROR);
            return true;
        }()));

        SUBTEST("comma_delimited_array missing trailing bracket is TAPE_ERROR", ([&]() {
            auto input = R"([1,2,3)"_padded;
            auto err = parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream);
            ASSERT_ERROR(err, simdjson::TAPE_ERROR);
            return true;
        }()));

        SUBTEST("comma_delimited_array empty input is TAPE_ERROR", ([&]() {
            auto input = ""_padded;
            auto err = parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream);
            ASSERT_ERROR(err, simdjson::TAPE_ERROR);
            return true;
        }()));

        SUBTEST("comma_delimited_array whitespace-only input is TAPE_ERROR", ([&]() {
            auto input = "   \t\n"_padded;
            auto err = parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream);
            ASSERT_ERROR(err, simdjson::TAPE_ERROR);
            return true;
        }()));

        SUBTEST("comma_delimited_array multibatch large array", ([&]() {
            // Generate [0,1,2,...,N-1] and iterate with a small batch_size
            // so the stream runs stage1 across many windows. All N values
            // must come out in order with no phantom documents.
            constexpr size_t N = 256;
            std::string bytes = "[";
            for (size_t i = 0; i < N; i++) {
                if (i > 0) { bytes += ','; }
                bytes += std::to_string(i);
            }
            bytes += "]";
            auto input = simdjson::padded_string(bytes);
            ASSERT_SUCCESS(parser.iterate_many(input, 64, simdjson::stream_format::comma_delimited_array).get(stream));
            size_t count = 0;
            for (auto it = stream.begin(); it != stream.end(); ++it) {
                auto doc_res = *it;
                ASSERT_SUCCESS(doc_res.error());
                ASSERT_TRUE(count < N);
                ondemand::document_reference doc;
                ASSERT_SUCCESS(doc_res.get(doc));
                int64_t v;
                ASSERT_SUCCESS(doc.get_int64().get(v));
                ASSERT_EQUAL(v, int64_t(count));
                count++;
            }
            ASSERT_EQUAL(count, N);
            return true;
        }()));

        TEST_SUCCEED();
    }

#if SIMDJSON_STATIC_REFLECTION
    // A simple type whose deserialization is generated by C++26 static
    // reflection. No tag_invoke is provided on purpose.
    struct ReflCar {
        std::string make{};
        std::string model{};
        int64_t year{};
        std::vector<double> tire_pressure{};
        bool operator==(const ReflCar &other) const {
            return make == other.make && model == other.model &&
                   year == other.year && tire_pressure == other.tire_pressure;
        }
    };

    static const std::vector<ReflCar> expected_refl_cars = {
        {"Toyota", "Camry",  2018, {40.1, 39.9}},
        {"Kia",    "Soul",   2012, {30.1, 31.0}},
        {"Toyota", "Tercel", 1999, {29.8, 30.0}},
    };

    static bool check_refl_cars(const std::vector<ReflCar> &cars) {
        ASSERT_EQUAL(cars.size(), expected_refl_cars.size());
        for (size_t i = 0; i < cars.size(); i++) {
            ASSERT_TRUE(cars[i] == expected_refl_cars[i]);
        }
        return true;
    }

    bool reflection_whitespace_delimited_stream() {
        TEST_START();
        auto json = R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
                          "tire_pressure": [ 40.1, 39.9 ] }
                        { "make": "Kia",    "model": "Soul",   "year": 2012,
                          "tire_pressure": [ 30.1, 31.0 ] }
                        { "make": "Toyota", "model": "Tercel", "year": 1999,
                          "tire_pressure": [ 29.8, 30.0 ] } )"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json, ondemand::DEFAULT_BATCH_SIZE,
                                           simdjson::stream_format::whitespace_delimited).get(stream));
        std::vector<ReflCar> cars;
        for (auto doc : stream) {
            ReflCar c;
            ASSERT_SUCCESS(doc.get<ReflCar>().get(c));
            cars.push_back(std::move(c));
        }
        if (!check_refl_cars(cars)) { return false; }
        TEST_SUCCEED();
    }

    bool reflection_comma_delimited_stream() {
        TEST_START();
        auto json = R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
                          "tire_pressure": [ 40.1, 39.9 ] },
                        { "make": "Kia",    "model": "Soul",   "year": 2012,
                          "tire_pressure": [ 30.1, 31.0 ] },
                        { "make": "Toyota", "model": "Tercel", "year": 1999,
                          "tire_pressure": [ 29.8, 30.0 ] } )"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json, ondemand::DEFAULT_BATCH_SIZE,
                                           simdjson::stream_format::comma_delimited).get(stream));
        std::vector<ReflCar> cars;
        for (auto doc : stream) {
            ReflCar c;
            ASSERT_SUCCESS(doc.get<ReflCar>().get(c));
            cars.push_back(std::move(c));
        }
        if (!check_refl_cars(cars)) { return false; }
        TEST_SUCCEED();
    }

    bool reflection_comma_delimited_array_stream() {
        TEST_START();
        auto json = R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
                            "tire_pressure": [ 40.1, 39.9 ] },
                          { "make": "Kia",    "model": "Soul",   "year": 2012,
                            "tire_pressure": [ 30.1, 31.0 ] },
                          { "make": "Toyota", "model": "Tercel", "year": 1999,
                            "tire_pressure": [ 29.8, 30.0 ] } ] )"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json, ondemand::DEFAULT_BATCH_SIZE,
                                           simdjson::stream_format::comma_delimited_array).get(stream));
        std::vector<ReflCar> cars;
        for (auto doc : stream) {
            ReflCar c;
            ASSERT_SUCCESS(doc.get<ReflCar>().get(c));
            cars.push_back(std::move(c));
        }
        if (!check_refl_cars(cars)) { return false; }
        TEST_SUCCEED();
    }

    bool reflection_json_sequence_stream() {
        TEST_START();
        // Build the input with RS (0x1E) prefix and LF (0x0A) trailer
        // for each document, per RFC 7464.
        std::string input_str;
        auto append = [&](std::string_view doc) {
            input_str += '\x1e';
            input_str += doc;
            input_str += '\x0a';
        };
        append(R"({ "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9 ] })");
        append(R"({ "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0 ] })");
        append(R"({ "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0 ] })");
        simdjson::padded_string input(input_str);

        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE,
                                           simdjson::stream_format::json_sequence).get(stream));
        std::vector<ReflCar> cars;
        for (auto doc : stream) {
            ReflCar c;
            ASSERT_SUCCESS(doc.get<ReflCar>().get(c));
            cars.push_back(std::move(c));
        }
        if (!check_refl_cars(cars)) { return false; }
        TEST_SUCCEED();
    }

    bool reflection_stream_tests() {
        return reflection_whitespace_delimited_stream() &&
               reflection_comma_delimited_stream() &&
               reflection_comma_delimited_array_stream() &&
               reflection_json_sequence_stream();
    }
#endif // SIMDJSON_STATIC_REFLECTION

    bool run() {
        return
#if SIMDJSON_STATIC_REFLECTION
            reflection_stream_tests() &&
#endif
            json_sequence_tests() &&
            comma_delimited_tests() &&
            issue2181() &&
            issue2170() &&
            issue_non_ascii_separator_source() &&
            issue2137() &&
            skipbom() &&
            issue1977() &&
            string_with_trailing() &&
            uint64_with_trailing() &&
            int64_with_trailing() &&
            uint32_with_trailing() &&
            int32_with_trailing() &&
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
            simple_document_iteration_advance() &&
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
