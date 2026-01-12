#include "../singleheader/simdjson.h"
#include <iostream>
#include <string_view>
#include <vector>

using namespace simdjson;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    const char *filename = argv[1];

    // 3. Memory management & Padding
    // Explicitly use padded_string to ensure input buffer has correct padding for SIMD
    padded_string json;
    auto error = padded_string::load(filename).get(json);
    if (error) {
        std::cerr << "Error loading file: " << error << std::endl;
        return 1;
    }

    // 1. On-Demand API
    ondemand::parser parser;
    ondemand::document_stream docs;
    
    // 2. Stream iteration
    // Use iterate_many for NDJSON to enable batch processing
    error = parser.iterate_many(json).get(docs);
    if (error) {
        std::cerr << "Error starting iteration: " << error << std::endl;
        return 1;
    }

    double total_latency = 0;
    size_t count = 0;

    // Iterate over the documents
    for (auto doc_result : docs) {
        // 5. Robust error handling
        if (doc_result.error()) {
            // Skip malformed JSON lines
            continue; 
        }
        
        ondemand::object obj;
        if (doc_result.get_object().get(obj)) {
            continue;
        }

        std::string_view service;
        int64_t status = -1;
        double latency = -1.0;
        bool service_found = false;
        bool status_found = false;
        bool latency_found = false;

        // Iterate over fields to handle arbitrary order
        for (auto field : obj) {
            std::string_view key;
            if (field.unescaped_key().get(key)) {
                continue; // Skip fields with invalid keys
            }
            
            if (key == "service") {
                if (!field.value().get_string().get(service)) {
                    service_found = true;
                }
            } else if (key == "status") {
                if (!field.value().get_int64().get(status)) {
                    status_found = true;
                }
            } else if (key == "latency_ms") {
                if (!field.value().get_double().get(latency)) {
                    latency_found = true;
                }
            }
        }

        // Check if we have all required fields and they match our criteria
        if (service_found && status_found && latency_found) {
            // 4. Zero-copy & String processing
            // service is std::string_view, zero-copy comparison
            if (service == "payment-api" && status == 200) {
                total_latency += latency;
                count++;
            }
        }
    }

    if (count > 0) {
        printf("%.2f\n%zu\n", total_latency / count, count);
    } else {
        printf("0.00\n0\n");
    }

    return 0;
}
