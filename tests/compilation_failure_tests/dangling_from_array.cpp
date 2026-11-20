// simdjson::from(json) returns a temporary that holds the document. An array
// taken from it points into that document, which is destroyed at the end of
// the full-expression: before C++23, even a range-based for loop over
// simdjson::from(json).array() reads freed memory. It must not compile.

#include <cstdlib>
#include <iostream>

#include "simdjson.h"

#if SIMDJSON_SUPPORTS_CONCEPTS
int main() {
    simdjson::padded_string json(std::string("[1, 2, 3]"));
    int64_t sum = 0;
#if COMPILATION_TEST_USE_FAILING_CODE
    for (auto value : simdjson::from(json).array()) {
#else
    auto parsed = simdjson::from(json);
    for (auto value : parsed.array()) {
#endif
        int64_t x{};
        if (value.get(x)) { return EXIT_FAILURE; }
        sum += x;
    }
    std::cout << sum << std::endl;
    return sum == 6 ? EXIT_SUCCESS : EXIT_FAILURE;
}
#else
#if COMPILATION_TEST_USE_FAILING_CODE
#error "simdjson::from needs concepts: report the overload as deleted"
#endif
int main() { return EXIT_SUCCESS; }
#endif
