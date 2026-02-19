#include "mylib.h"
#include <simdjson.h>
#include <iostream>
#include <memory>

int main() {
    simdjson::padded_string json = R"([ 1, 2, 3, 4 ])"_padded;
    simdjson::padded_string minified = minify_json(json);
    std::cout << "Minified: " << std::string_view(minified) << std::endl;

    // Also directly use minify
    size_t length = json.size();
    std::unique_ptr<char[]> buffer{new char[length]};
    size_t new_length{};
    auto error = simdjson::minify(json.data(), length, buffer.get(), new_length);
    if (error) {
        std::cerr << "Error: " << simdjson::error_message(error) << std::endl;
    } else {
        std::cout << "Direct minified: " << std::string(buffer.get(), new_length) << std::endl;
    }
    return 0;
}