#include <iostream>
#include <string>
#include <vector>
#include "stream_parser.h"

void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [options] [file]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --config <file>   Path to configuration file" << std::endl;
    std::cerr << "  --follow          Follow file (tail -f mode)" << std::endl;
    std::cerr << "  --benchmark       Benchmark mode (no alerting)" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string config_file;
    std::string input_file;
    bool follow = false;
    bool benchmark = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config") {
            if (i + 1 < argc) config_file = argv[++i];
        } else if (arg == "--follow") {
            follow = true;
        } else if (arg == "--benchmark") {
            benchmark = true;
        } else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            input_file = arg;
        }
    }

    StreamParser parser;
    size_t count = 0;
    
    auto callback = [&](simdjson::ondemand::document& doc, std::string_view raw_json) {
        count++;
        if (!benchmark) {
            // In real implementation, we would pass doc to AnomalyDetector
            // For now, just print a dot every 1000 events
            if (count % 1000 == 0) {
                std::cout << "." << std::flush;
            }
        }
    };

    std::cout << "Starting anomaly detector..." << std::endl;
    if (input_file.empty() || input_file == "-") {
        parser.parse_stdin(callback);
    } else {
        parser.parse_file(input_file, follow, callback);
    }
    std::cout << "\nProcessed " << count << " events." << std::endl;

    return 0;
}
