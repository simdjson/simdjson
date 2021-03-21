
// this tests https://github.com/simdjson/simdjson/issues/696

#include <iostream>
#include "simdjson.h"

int main() {
#if COMPILATION_TEST_USE_FAILING_CODE
    simdjson::dom::element tree = simdjson::dom::parser().load("tree-pretty.json");
#else
    simdjson::dom::parser parser;
    simdjson::dom::element tree = parser.load("tree-pretty.json");
#endif
    std::cout << tree["type"] << std::endl;
}
