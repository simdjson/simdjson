#include <iostream>
#include "simdjson.h"
using namespace simdjson;
int main(void) {
    padded_string json;
    auto error = padded_string::load("twitter.json").get(json);
    if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }

    ondemand::parser parser;
    ondemand::document tweets;
    error = parser.iterate(json).get(tweets);
    if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }

    uint64_t count;
    error = tweets["search_metadata"]["count"].get(count);
    if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }

    std::cout << count << " results." << std::endl;

    return EXIT_SUCCESS;
}
