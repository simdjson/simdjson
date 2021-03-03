#include <iostream>
#include "simdjson.h"

using namespace simdjson;

int main() {
    ondemand::parser parser;
#if COMPILATION_TEST_USE_FAILING_CODE
    auto json = std::string("1");
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
