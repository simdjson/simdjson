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
const double MAX_TRIAL_COUNT= 5;

void run_tests(const std::string refCommand, const std::string newCommand, double &worseref, double &bestref, double&worsenewcode, double &bestnewcode) {
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
    worseref = *std::min_element(ref.begin(), ref.end());
    bestnewcode =  *std::max_element(newcode.begin(), newcode.end());
    bestref = *std::max_element(ref.begin(), ref.end());
    worsenewcode =  *std::min_element(newcode.begin(), newcode.end());
    std::cout << "The new code has a throughput in       " << worsenewcode << " -- " << bestnewcode << std::endl;
    std::cout << "The reference code has a throughput in " << worseref << " -- " << bestref << std::endl;
}


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
    double worseref, bestref, worsenewcode, bestnewcode;
    /**
     * We take performance degradation seriously. When it occurs, we want
     * to investigate it thoroughly. Theoretically, if INTERLEAVED_ATTEMPTS
     * samples from one distribution are distinct from INTERLEAVED_ATTEMPTS
     * from another distribution, then there should be a real difference.
     * Unfortunately, in practice, we can get the impression that there are
     * false positives. So the tool should make absolutely sure that the
     * difference is entirely reproducible. So we require that it be
     * able to reproduce it consistently MAX_TRIAL_COUNT times. Then it
     * will be hard to argue with.
     */
    int degradation = 0;
    int gain = 0;
    int neutral = 0;

    // at most, we will rerun the tests MAX_TRIAL_COUNT times
    for(size_t trial = 0 ; trial < MAX_TRIAL_COUNT; trial++) {
      run_tests(refCommand, newCommand, worseref, bestref, worsenewcode, bestnewcode);
      if(bestnewcode < worseref) {
        printf("Possible degradation detected (%f %%)\n", (worseref - bestnewcode) * 100.0 / worseref);
        degradation++;
        if(gain > 0) {
            break; // mixed results
        }
        // otherwise, continue to make sure that the bad result is not a fluke
      } else if(bestref < worsenewcode) {
        printf("Possible gain detected (%f %%)\n", (bestref - bestref) * 100.0 / bestref);
        gain++;
        if(degradation > 0) {
            break; // mixed results
        }
        // otherwise, continue to make sure that the good result is not a fluke
      } else {
        // Whenever no difference is detected, we cut short.
        neutral++;
        break;
      }
    }
    // If we have at least one neutral, we conclude that there is no difference.
    // If we have mixed results,  we conclude that there is no difference.
    if(neutral > 0 || ((gain > 0) && (degradation > 0)) ){
        std::cout << "There may not be performance difference. A manual check might be needed." << std::endl;
        return EXIT_SUCCESS;
    }
    if(gain > 0) {
        std::cout << "You may have a performance gain." << std::endl;
        return EXIT_SUCCESS;
    }

    std::cerr << "You probably have a performance degradation." << std::endl;
    return EXIT_FAILURE;
}
