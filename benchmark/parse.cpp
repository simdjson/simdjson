#include "json_parser.h"
#include "event_counter.h"

#include <cassert>
#include <cctype>
#ifndef _MSC_VER
#include <dirent.h>
#include <unistd.h>
#endif
#include <cinttypes>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "linux-perf-events.h"
#ifdef __linux__
#include <libgen.h>
#endif
//#define DEBUG
#include "simdjson/common_defs.h"
#include "simdjson/isadetection.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/jsonparser.h"
#include "simdjson/parsedjson.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_build_tape.h"

#include <functional>

#include "benchmarker.h"

using namespace simdjson;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::to_string;
using std::vector;
using std::ostream;
using std::ofstream;
using std::exception;

// Stash the exe_name in main() for functions to use
char* exe_name;

void print_usage(ostream& out) {
  out << "Usage: " << exe_name << " [-vt] [-n #] [-s STAGE] [-a ARCH] <jsonfile> ..." << endl;
  out << endl;
  out << "Runs the parser against the given json files in a loop, measuring speed and other statistics." << endl;
  out << endl;
  out << "Options:" << endl;
  out << endl;
  out << "-n #       - Number of iterations per file. Default: 200" << endl;
  out << "-i #       - Number of times to iterate a single file before moving to the next. Default: 20" << endl;
  out << "-t         - Tabbed data output" << endl;
  out << "-v         - Verbose output." << endl;
  out << "-s STAGE   - Stop after the given stage." << endl;
  out << "             -s stage1  - Stop after find_structural_bits." << endl;
  out << "             -s all     - Run all stages." << endl;
  out << "-H         - Make the buffers hot (reduce page allocation during parsing)" << endl;

  out << "-a ARCH    - Use the parser with the designated architecture (HASWELL, WESTMERE" << endl;
  out << "             or ARM64). By default, detects best supported architecture." << endl;
}

void exit_usage(string message) {
  cerr << message << endl;
  cerr << endl;
  print_usage(cerr);
  exit(EXIT_FAILURE);
}

struct option_struct {
  vector<char*> files;
  architecture arch = architecture::UNSUPPORTED;
  bool stage1_only = false;

  int32_t iterations = 200;
  int32_t iteration_step = 50;

  bool verbose = false;
  bool tabbed_output = false;
  bool hotbuffers = false;

  option_struct(int argc, char **argv) {
    #ifndef _MSC_VER
      int c;

      while ((c = getopt(argc, argv, "vtn:i:a:s:H")) != -1) {
        switch (c) {
        case 'n':
          iterations = atoi(optarg);
          break;
        case 'i':
          iteration_step = atoi(optarg);
          break;
        case 't':
          tabbed_output = true;
          break;
        case 'v':
          verbose = true;
          break;
        case 'a':
          arch = parse_architecture(optarg);
          if (arch == architecture::UNSUPPORTED) {
            exit_usage(string("Unsupported option value -a ") + optarg + ": expected -a HASWELL, WESTMERE or ARM64");
          }
          break;
        case 'H':
          hotbuffers = true;
          break;
        case 's':
          if (!strcmp(optarg, "stage1")) {
            stage1_only = true;
          } else if (!strcmp(optarg, "all")) {
            stage1_only = false;
          } else {
            exit_usage(string("Unsupported option value -s ") + optarg + ": expected -s stage1 or all");
          }
          break;
        default:
          // reaching here means an argument was given to getopt() which did not have a case label
          exit_error("Unexpected argument - missing case for option "+
                     std::string(1,static_cast<char>(c))+
                     " (programming error)");
        }
      }
    #else
      int optind = 1;
    #endif

    // If architecture is not specified, pick the best supported architecture by default
    if (arch == architecture::UNSUPPORTED) {
      arch = find_best_supported_architecture();
    }

    // All remaining arguments are considered to be files
    for (int i=optind; i<argc; i++) {
      files.push_back(argv[i]);
    }
    if (files.empty()) {
      exit_usage("No files specified");
    }

    // Keeps the numbers the same for CI (old ./parse didn't have a two-stage loop)
    if (files.size() == 1) {
      iteration_step = iterations;
    }

    #if !defined(__linux__)
      if (tabbed_output) {
        exit_error("tabbed_output (-t) flag only works under linux.\n");
      }
    #endif
  }
};

int main(int argc, char *argv[]) {
  // Read options
  exe_name = argv[0];
  option_struct options(argc, argv);
  if (options.verbose) {
    verbose_stream = &cout;
  }

  // Start collecting events. We put this early so if it prints an error message, it's the
  // first thing printed.
  event_collector collector;

  // Print preamble
  if (!options.tabbed_output) {
    printf("number of iterations %u \n", options.iterations);
  }

  // Set up benchmarkers by reading all files
  json_parser parser(options.arch);
  vector<benchmarker*> benchmarkers;
  for (size_t i=0; i<options.files.size(); i++) {
    benchmarkers.push_back(new benchmarker(options.files[i], parser, collector));
  }

  // Run the benchmarks
  progress_bar progress(options.iterations, 50);
  // Put the if (options.stage1_only) *outside* the loop so that run_iterations will be optimized
  if (options.stage1_only) {
    for (int iteration = 0; iteration < options.iterations; iteration += options.iteration_step) {
      if (!options.verbose) { progress.print(iteration); }
      // Benchmark each file once per iteration
      for (size_t f=0; f<options.files.size(); f++) {
        verbose() << "[verbose] " << benchmarkers[f]->filename << " iterations #" << iteration << "-" << (iteration+options.iteration_step-1) << endl;
        benchmarkers[f]->run_iterations(options.iteration_step, true, options.hotbuffers);
      }
    }
  } else {
    for (int iteration = 0; iteration < options.iterations; iteration += options.iteration_step) {
      if (!options.verbose) { progress.print(iteration); }
      // Benchmark each file once per iteration
      for (size_t f=0; f<options.files.size(); f++) {
        verbose() << "[verbose] " << benchmarkers[f]->filename << " iterations #" << iteration << "-" << (iteration+options.iteration_step-1) << endl;
        benchmarkers[f]->run_iterations(options.iteration_step, false, options.hotbuffers);
      }
    }
  }
  if (!options.verbose) { progress.erase(); }

  for (size_t i=0; i<options.files.size(); i++) {
    benchmarkers[i]->print(options.tabbed_output);
    delete benchmarkers[i];
  }

  return EXIT_SUCCESS;
}
