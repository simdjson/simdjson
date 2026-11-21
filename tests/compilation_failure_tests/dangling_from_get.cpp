// An ondemand::object read with get() from the temporary returned by
// simdjson::from(json) points into the document held by that temporary,
// which is destroyed at the end of the full-expression. It must not compile.

#include <cstdlib>
#include <iostream>

#include "simdjson.h"

#if SIMDJSON_SUPPORTS_CONCEPTS
int main() {
    simdjson::padded_string json(std::string(R"({"a": 1, "b": 2})"));
    simdjson::ondemand::object object;
#if COMPILATION_TEST_USE_FAILING_CODE
    if (simdjson::from(json).get(object)) { return EXIT_FAILURE; }
#else
    auto parsed = simdjson::from(json);
    if (parsed.result<simdjson::ondemand::object>().get(object)) { return EXIT_FAILURE; }
#endif
    int64_t sum = 0;
    for (auto field : object) {
        int64_t x{};
        if (field.value().get(x)) { return EXIT_FAILURE; }
        sum += x;
    }
    std::cout << sum << std::endl;
    return sum == 3 ? EXIT_SUCCESS : EXIT_FAILURE;
}
#else
#if COMPILATION_TEST_USE_FAILING_CODE
#error "simdjson::from needs concepts: report the overload as deleted"
#endif
int main() { return EXIT_SUCCESS; }
#endif
