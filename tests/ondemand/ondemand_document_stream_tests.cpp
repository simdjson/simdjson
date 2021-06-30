#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace document_stream_tests {
    bool testing() {
        TEST_START();
        auto json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3])"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ondemand::document_stream::iterator i;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        i = stream.begin();
        doc = *i;
        std::cout << doc.iter.peek() << std::endl;  // Prints [1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3]
        ++i;
        doc = *i;
        std::cout << doc.iter.peek() << std::endl;  // Prints {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3]
        ++i;
        doc = *i;
        std::cout << doc.iter.peek() << std::endl;  // Prints {"o":{"1":1,"2":2}} [1,2,3]
        ++i;
        doc = *i;
        std::cout << doc.iter.peek() << std::endl;  // Prints [1,2,3]
        TEST_SUCCEED();
    }

    bool actual() {
        TEST_START();
        auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} [1,2,3]  )"_padded;
        dom::parser parser;
        dom::document_stream stream;
        dom::document_stream::iterator i;
        ASSERT_SUCCESS(parser.parse_many(json).get(stream));
        i = stream.begin();
        auto doc = *i;
        std::cout << doc << std::endl;
        //std::cout << i.stream->doc_index << std::endl;
        //std::cout << i.stream->parser->implementation->next_structural_index << std::endl;
        TEST_SUCCEED()
    }

    bool run() {
        return
                testing() &&
                actual() &&
                true;
    }
} // document_stream_tests

int main (int argc, char *argv[]) {
    return test_main(argc, argv, document_stream_tests::run);
}