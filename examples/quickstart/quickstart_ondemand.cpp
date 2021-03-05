#include "simdjson.h"
using namespace simdjson;
int main(void) {
    parser parser;
    padded_string json = padded_string::load("twitter.json");
    document tweets = parser.iterate(json);
    std::cout << uint64_t(tweets["search_metadata"]["count"]) << " results." << std::endl;
}
