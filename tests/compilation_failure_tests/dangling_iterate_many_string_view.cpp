// A std::string_view converts implicitly to a padded_string. Passing one to
// iterate_many() therefore materializes a temporary that is destroyed at the end of
// the full-expression, while the returned document_stream keeps a pointer to it:
// iterating the stream afterwards reads freed memory. It must not compile.

#include <string>
#include <string_view>
#include <iostream>

#include "simdjson.h"

int main() {
    std::string input = R"({"hello": "world"} {"hello": "there"})";
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document_stream stream;
#if COMPILATION_TEST_USE_FAILING_CODE
    std::string_view view(input);
    auto error = parser.iterate_many(view).get(stream);
#else
    simdjson::padded_string json(input);
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
