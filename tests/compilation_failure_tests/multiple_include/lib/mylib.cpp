#include "mylib.h"
#include <simdjson.h>
#include <memory>

simdjson::padded_string minify_json(const simdjson::padded_string& json) {
    size_t length = json.size();
    std::unique_ptr<char[]> buffer{new char[length]};
    size_t new_length{};
    auto error = simdjson::minify(json.data(), length, buffer.get(), new_length);
    if (error) {
        return simdjson::padded_string();
    }
    return simdjson::padded_string(std::string_view(buffer.get(), new_length));
}