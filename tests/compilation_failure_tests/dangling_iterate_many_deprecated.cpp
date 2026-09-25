// The deprecated allow_comma_separated overloads of iterate_many() must reject a
// temporary input too: the returned document_stream would keep a pointer to it after
// it is destroyed at the end of the full-expression. It must not compile.

#include <string>
#include <iostream>

#include "simdjson.h"

int main() {
    std::string input = R"({"hello": "world"} {"hello": "there"})";
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document_stream stream;
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
#if COMPILATION_TEST_USE_FAILING_CODE
    auto error = parser.iterate_many(std::string(input), 65536, true).get(stream);
#else
    auto error = parser.iterate_many(input, 65536, true).get(stream);
#endif
SIMDJSON_POP_DISABLE_WARNINGS
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
