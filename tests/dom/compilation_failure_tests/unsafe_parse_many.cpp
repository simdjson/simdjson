#include <string>
#include <vector>
#include <iostream>

#include "simdjson.h"

bool single_document() {
    std::cout << "Running " << __func__ << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;

#if COMPILATION_TEST_USE_FAILING_CODE
    auto error = parser.parse_many(json).get(R"({"hello": "world"})"_padded);
#else
    auto json = R"({"hello": "world"})"_padded;
    auto error = parser.parse_many(json).get(stream);
#endif
    if(error) {
        std::cerr << error << std::endl;
        return false;
    }
    size_t count = 0;
    for (auto doc : stream) {
        if(doc.error()) {
          std::cerr << "Unexpected error: " << doc.error() << std::endl;
          return false;
        }
        std::string expected = R"({"hello":"world"})";
        simdjson::dom::element this_document;
        error = doc.get(this_document);
        if(error) {
          std::cerr << error << std::endl;
          return false;
        }
        std::string answer = simdjson::minify(this_document);
        if(answer != expected) {
          std::cout << this_document << std::endl;
          return false;
        }
        count += 1;
    }
    return count == 1;
}

int main() {
    return single_document() ? EXIT_SUCCESS : EXIT_FAILURE;
}