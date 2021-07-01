#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace document_stream_tests {

    std::string my_string(ondemand::document& doc) {
        std::stringstream ss;
        ss << doc;
        return ss.str();
    }

    bool testing() {
        TEST_START();
        auto json = R"([1,[1,2]] {"a":1,"b":2} {"o":{"1":1,"2":2}} [1,2,3])"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ondemand::document_stream::iterator i;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        i = stream.begin();
        ASSERT_EQUAL(my_string(*i), "[1,[1,2]]");
        ++i;
        ASSERT_EQUAL(my_string(*i), "{\"a\":1,\"b\":2}");
        ++i;
        ASSERT_EQUAL(my_string(*i), "{\"o\":{\"1\":1,\"2\":2}}");
        ++i;
        ASSERT_EQUAL(my_string(*i), "[1,2,3]");
        TEST_SUCCEED();
    }

    bool run() {
        return
                testing() &&
                true;
    }
} // document_stream_tests

int main (int argc, char *argv[]) {
    return test_main(argc, argv, document_stream_tests::run);
}