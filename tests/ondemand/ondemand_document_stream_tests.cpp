#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace document_stream_tests {
    bool testing() {
        TEST_START();
        auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} [1,2,3]  )"_padded;
        ondemand::parser parser;
        ondemand::document_stream stream;
        ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
        std::cout << "OK" << std::endl;
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