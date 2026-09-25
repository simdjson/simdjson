// A std::string temporary passed to iterate_many() is destroyed at the end of the
// full-expression, while the returned document_stream keeps a pointer to it:
// iterating the stream afterwards reads freed memory. It must not compile.

#include <string>
#include <iostream>

#include "simdjson.h"

int main() {
    std::string input = R"({"hello": "world"} {"hello": "there"})";
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document_stream stream;
#if COMPILATION_TEST_USE_FAILING_CODE
    auto error = parser.iterate_many(std::string(input)).get(stream);
#else
    auto error = parser.iterate_many(input).get(stream);
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
