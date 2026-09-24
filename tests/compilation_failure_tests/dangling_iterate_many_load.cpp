// padded_string::load() returns a simdjson_result<padded_string> temporary. Passing it
// directly to iterate_many() leaves the returned document_stream pointing at a
// padded_string that is destroyed at the end of the full-expression: iterating the
// stream afterwards reads freed memory. It must not compile.

#include <string>
#include <iostream>

#include "simdjson.h"

int main() {
    std::string input = R"({"hello": "world"} {"hello": "there"})";
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document_stream stream;
#if COMPILATION_TEST_USE_FAILING_CODE
    auto error = parser.iterate_many(simdjson::padded_string::load("stream.json")).get(stream);
#else
    simdjson::padded_string json;
    if (simdjson::padded_string::load("stream.json").get(json)) { json = simdjson::padded_string(input); }
    auto error = parser.iterate_many(json).get(stream);
#endif
    if (error) {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }
    for (auto doc : stream) {
        if (doc.error()) {
            std::cerr << doc.error() << std::endl;
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
