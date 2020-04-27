#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <array>
#include <algorithm>
#include <vector>
#include <cmath>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

int closepipe(FILE *pipe) {
    int exit_code = pclose(pipe);
    if (exit_code != EXIT_SUCCESS) {
        std::cerr << "Error " << exit_code << " running benchmark command!" << std::endl;
        exit(EXIT_FAILURE);
    };
    return exit_code;
}

std::string exec(const char* cmd) {
    std::cerr << cmd << std::endl;
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&closepipe)> pipe(popen(cmd, "r"), closepipe);
    if (!pipe) {
        std::cerr << "popen() failed!" << std::endl;
        abort();
    }
    while (fgets(buffer.data(), int(buffer.size()), pipe.get()) != nullptr) {
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
        std::string::size_type pos = 0;
        for (int i=0; i<5; i++) {
            pos = line.find('\t', pos);
            if (pos == std::string::npos) {
                std::cerr << "Command printed out a line with less than 5 fields in it:\n" << line << std::endl;
            }
            pos++;
        }
        result += std::stod(line.substr(pos));
        numResults++;
    }
    if (numResults == 0) {
        std::cerr << "No results returned from benchmark command!" << std::endl;
        exit(EXIT_FAILURE);
    }
    return result / numResults;
}

const double INTERLEAVED_ATTEMPTS = 7;

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <old parse exe> <new parse exe> [<parse arguments>]" << std::endl;
        return 1;
    }

    std::string newCommand = argv[1];
    std::string refCommand = argv[2];
    for (int i=3; i<argc; i++) {
        newCommand += " ";
        newCommand += argv[i];
        refCommand += " ";
        refCommand += argv[i];
    }

    std::vector<double> ref;
    std::vector<double> newcode;
    for (int attempt=0; attempt < INTERLEAVED_ATTEMPTS; attempt++) {
        std::cout << "Attempt #" << (attempt+1) << " of " << INTERLEAVED_ATTEMPTS << std::endl;

        // Read new throughput
        double newThroughput = readThroughput(exec(newCommand.c_str()));
        std::cout << "New throughput: " << newThroughput << std::endl;
        newcode.push_back(newThroughput);

        // Read reference throughput
        double referenceThroughput = readThroughput(exec(refCommand.c_str()));
        std::cout << "Ref throughput: " << referenceThroughput << std::endl;
        ref.push_back(referenceThroughput);
    }
    // we check if the maximum of newcode is lower than minimum of ref, if so we have a problem so fail!
    double worseref = *std::min_element(ref.begin(), ref.end());
    double bestnewcode =  *std::max_element(newcode.begin(), newcode.end());
    double bestref = *std::max_element(ref.begin(), ref.end());
    double worsenewcode =  *std::min_element(newcode.begin(), newcode.end());
    std::cout << "The new code has a throughput in       " << worsenewcode << " -- " << bestnewcode << std::endl;
    std::cout << "The reference code has a throughput in " << worseref << " -- " << bestref << std::endl;
    if(bestnewcode < worseref) {
      std::cerr << "You probably have a performance degradation." << std::endl;
      return EXIT_FAILURE;
    }
    if(bestnewcode < worseref) {
      std::cout << "You probably have a performance gain." << std::endl;
      return EXIT_SUCCESS;
    }
    std::cout << "There is no obvious performance difference. A manual check might be needed." << std::endl;
    return EXIT_SUCCESS;
}
