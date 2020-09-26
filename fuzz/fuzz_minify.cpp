#include "simdjson.h"
#include "FuzzUtils.h"
#include <cstddef>
#include <cstdint>
#include <string>

/*
 * Minifies by first parsing, then minifying.
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    auto begin = as_chars(Data);
    auto end = begin + Size;

    std::string str(begin, end);
    simdjson::dom::parser parser;
    simdjson::dom::element elem;
    auto error = parser.parse(str).get(elem);
    if (error) { return 0; }

    std::string minified=simdjson::minify(elem);
    (void)minified;
    return 0;
}
