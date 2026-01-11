#include "stream_parser.h"
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>

StreamParser::StreamParser(size_t buffer_size) 
    : buffer_size_(buffer_size) {
    // Allocate buffer with padding
    buffer_.resize(buffer_size_ + simdjson::SIMDJSON_PADDING);
}

void StreamParser::parse_stdin(Callback callback) {
    size_t offset = 0;
    while (true) {
        // Read into buffer starting at offset
        // We can read up to buffer_size_ - offset
        if (offset >= buffer_size_) {
            std::cerr << "Buffer full, document too large or stuck?" << std::endl;
            // Reset offset to avoid infinite loop, but this means data loss
            offset = 0; 
        }

        ssize_t bytes_read = read(STDIN_FILENO, buffer_.data() + offset, buffer_size_ - offset);
        
        if (bytes_read < 0) {
            perror("read");
            break;
        }
        if (bytes_read == 0) {
            // EOF
            // Process remaining buffer if any
            if (offset > 0) {
                process_buffer(offset, callback, offset);
            }
            break;
        }

        process_buffer(offset + bytes_read, callback, offset);
    }
}

void StreamParser::process_buffer(size_t total_bytes, Callback callback, size_t& offset) {
    // simdjson requires padding to be accessible.
    // We pass the buffer as a padded_string_view.
    // Note: ondemand parser might modify the buffer during iteration.
    
    simdjson::padded_string_view psv(buffer_.data(), total_bytes, buffer_.size());
    
    auto stream = parser_.iterate_many(psv);
    
    for (auto it = stream.begin(); it != stream.end(); ++it) {
        simdjson::ondemand::document doc;
        auto error = (*it).get(doc);
        if (error == simdjson::SUCCESS) {
            callback(doc, it.source());
        } else {
            // If we encounter an error, it might be due to incomplete document at the end
            // or a real error.
            // iterate_many usually handles incomplete document by stopping and reporting truncated_bytes.
            // But if there is a syntax error in the middle, it might report it.
            // We log it and continue if possible.
            // std::cerr << "JSON error: " << error << std::endl;
        }
    }
    
    // Check truncated bytes
    size_t truncated = stream.truncated_bytes();
    
    // Move truncated bytes to the beginning
    if (truncated > 0 && truncated < total_bytes) {
        std::memmove(buffer_.data(), buffer_.data() + total_bytes - truncated, truncated);
        offset = truncated;
    } else if (truncated == total_bytes) {
        // Nothing was parsed, keep everything
        offset = total_bytes;
    } else {
        // Everything parsed
        offset = 0;
    }
}

void StreamParser::parse_file(const std::string& filename, bool follow, Callback callback) {
    // TODO: Implement file reading with follow support
    // For now, just use parse_stdin logic if filename is "-"
    // Or open file and read loop.
    
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    size_t offset = 0;
    while (true) {
        if (offset >= buffer_size_) {
             std::cerr << "Buffer full" << std::endl;
             offset = 0;
        }

        ssize_t bytes_read = read(fd, buffer_.data() + offset, buffer_size_ - offset);
        
        if (bytes_read < 0) {
            perror("read");
            close(fd);
            break;
        }
        
        if (bytes_read == 0) {
            if (follow) {
                // Sleep and retry
                usleep(100000); // 100ms
                continue;
            } else {
                // EOF
                if (offset > 0) {
                    process_buffer(offset, callback, offset);
                }
                break;
            }
        }

        process_buffer(offset + bytes_read, callback, offset);
    }
    close(fd);
}
