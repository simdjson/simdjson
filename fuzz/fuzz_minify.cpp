#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    auto begin = (const char *)Data;
    auto end = begin + Size;

    std::string str(begin, end);
    simdjson::dom::parser parser;
    simdjson::error_code error;
    simdjson::dom::element elem;
    parser.parse(str).tie(elem, error);
    if (error) { return 1; }

    std::string minified=simdjson::minify(elem);
    (void)minified;
    return 0;
}
