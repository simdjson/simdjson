#include <iostream>
#include "simdjson.h"

using namespace simdjson;

int main() {
    auto json = "1                                                                                          ";
#if COMPILATION_TEST_USE_FAILING_CODE;
    simdjson_unused padded_string_view p{json, 1};
#else
    simdjson_unused padded_string_view p{json, 1, strlen(json)};
#endif
    ondemand::parser parser;
#if COMPILATION_TEST_USE_FAILING_CODE
    auto json = std::string_view("1");
    auto doc = parser.iterate(json);
#else
    auto json = "1"_padded;
    auto doc = parser.iterate(json);
#endif
    int64_t value;
    auto error = doc.get(value);
    if (error) { exit(1); }
    std::cout << value << std::endl;
}
