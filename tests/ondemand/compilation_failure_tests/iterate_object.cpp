
#include <iostream>
#include "simdjson.h"

using namespace simdjson;

int main() {
    auto json = "{\"a\":1}"_padded;
    ondemand::parser parser;
    auto f = [](ondemand::parser& p, simdjson::padded_string& jsons) -> ondemand::document {
        ondemand::document doc;
        auto error = p.iterate(jsons).get(doc);
        if(error) { std::abort(); }
        return doc;
    };
    ondemand::object objv;
#if COMPILATION_TEST_USE_FAILING_CODE
    // Not allowed as this would be unsafe, the document must remain alive.
    auto error = f(parser).get_object().get(objv);
#else
    ondemand::document doc = f(parser, json);
    auto error = doc.get_object().get(objv);
#endif
    if(error) {
        std::cout << "Failure" << std::endl;
    }
    int64_t a = 0;
    error = objv["a"].get_int64().get(a);
    if(error) {
        std::cout << "failure" << std::endl;
    }
    printf("a = %d\n", (int)a);
    return EXIT_SUCCESS;
}