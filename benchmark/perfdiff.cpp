#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <array>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

double readThroughput(std::string parseOutput) {
    std::istringstream output(parseOutput);
    std::string line;
    double result = 0;
    int numResults = 0;
    while (std::getline(output, line)) {
        int pos = 0;
        for (int i=0; i<5; i++) {
            pos = line.find('\t', pos);
            if (pos < 0) {
                std::cerr << "Command printed out a line with less than 5 fields in it:\n" << line << std::endl;
            }
            pos++;
        }
        result += std::stod(line.substr(pos));
        numResults++;
    }
    return result / numResults;
}

const double INTERLEAVED_ATTEMPTS = 6;
const double PERCENT_DIFFERENCE_THRESHOLD = -0.2;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <new parse cmd> <reference parse cmd>" << std::endl;
        return 1;
    }
    for (int attempt=0; attempt < INTERLEAVED_ATTEMPTS; attempt++) {
        if (attempt > 0) {
            std::cout << "Running again to check whether it's a fluke ..." << std::endl;
        }
        std::cout << "Attempt #" << (attempt+1) << " of up to " << INTERLEAVED_ATTEMPTS << std::endl;

        // Read new throughput
        double newThroughput = readThroughput(exec(argv[1]));
        std::cout << "New throughput: " << newThroughput << std::endl;

        // Read reference throughput
        double referenceThroughput = readThroughput(exec(argv[2]));
        std::cout << "Ref throughput: " << referenceThroughput << std::endl;

        // Check if % difference > 0
        double percentDifference = ((newThroughput / referenceThroughput) - 1.0) * 100;
        std::cout << "Difference: " << percentDifference << "%" << std::endl;
        if (percentDifference >= PERCENT_DIFFERENCE_THRESHOLD) {
            std::cout << "New throughput is same or better." << std::endl;
            return 0;
        } else {
            std::cout << "New throughput is lower!";
        }
    }
    return 1;
}
