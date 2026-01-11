#pragma once

#include <simdjson.h>
#include <vector>
#include <string>
#include <functional>
#include <iostream>

class StreamParser {
public:
    using Callback = std::function<void(simdjson::ondemand::document&, std::string_view)>;

    StreamParser(size_t buffer_size = 1024 * 1024); // 1MB default

    // Parse from stdin continuously
    void parse_stdin(Callback callback);

    // Parse from a file (continuously if follow is true)
    void parse_file(const std::string& filename, bool follow, Callback callback);

private:
    size_t buffer_size_;
    std::vector<char> buffer_;
    simdjson::ondemand::parser parser_;

    void process_buffer(size_t bytes_read, Callback callback, size_t& offset);
};
