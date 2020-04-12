#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    auto begin = (const char *)Data;
    auto end = begin + Size;

    std::string str(begin, end);

    try {
        simdjson::dom::parser parser;
        simdjson::dom::element doc = parser.parse(str);
        std::string minified=simdjson::minify(doc);
        (void)minified;
    } catch (...) {

    }
    return 0;
}
