
// this tests https://github.com/simdjson/simdjson/issues/696

#include <iostream>
#include "simdjson.h"

int main() {
    std::string buf;
#if COMPILATION_TEST_USE_FAILING_CODE
    simdjson::dom::element tree = simdjson::dom::parser().parse(buf);
#else
    simdjson::dom::parser parser;
    simdjson::dom::element tree = parser.parse(buf);
#endif
    std::cout << tree["type"] << std::endl;
}
