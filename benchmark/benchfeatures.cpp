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
  out << "Usage: " << exe_name << " [-v] [-n #] [-s STAGE] [-a ARCH]" << endl;
  out << endl;
  out << "Runs the parser against jsonexamples/generated json files in a loop, measuring speed and other statistics." << endl;
  out << endl;
  out << "Options:" << endl;
  out << endl;
  out << "-n #       - Number of iterations per file. Default: 400" << endl;
  out << "-i #       - Number of times to iterate a single file before moving to the next. Default: 20" << endl;
  out << "-v         - Verbose output." << endl;
  out << "-s STAGE   - Stop after the given stage." << endl;
  out << "             -s stage1 - Stop after find_structural_bits." << endl;
  out << "             -s all    - Run all stages." << endl;
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
  Architecture architecture = Architecture::UNSUPPORTED;
  bool stage1_only = false;

  int32_t iterations = 400;
  int32_t iteration_step = 50;

  bool verbose = false;

  option_struct(int argc, char **argv) {
    #ifndef _MSC_VER
      int c;

      while ((c = getopt(argc, argv, "vtn:i:a:s:")) != -1) {
        switch (c) {
        case 'n':
          iterations = atoi(optarg);
          break;
        case 'i':
          iteration_step = atoi(optarg);
          break;
        case 'v':
          verbose = true;
          break;
        case 'a':
          architecture = parse_architecture(optarg);
          if (architecture == Architecture::UNSUPPORTED) {
            exit_usage(string("Unsupported option value -a ") + optarg + ": expected -a HASWELL, WESTMERE or ARM64");
          }
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
          exit_error("Unexpected argument " + c);
        }
      }
    #else
      int optind = 1;
    #endif

    // If architecture is not specified, pick the best supported architecture by default
    if (architecture == Architecture::UNSUPPORTED) {
      architecture = find_best_supported_architecture();
    }
  }
};

double actual(const benchmarker& feature) {
  return feature.stage1.best.elapsed_ns() / feature.stats->blocks;
}
double diff(const benchmarker& feature, const benchmarker& struct7) {
  if (feature.stats->blocks == struct7.stats->blocks) {
    return (feature.stage1.best.elapsed_ns() - struct7.stage1.best.elapsed_ns()) / struct7.stats->blocks;
  } else {
    return (feature.stage1.best.elapsed_ns() / feature.stats->blocks) - (struct7.stage1.best.elapsed_ns() / struct7.stats->blocks);
  }
}
double diff_miss(const benchmarker& feature, const benchmarker& struct7) {
  // There are roughly 2650 branch mispredicts, so we have to scale it so it represents a per block amount
  return diff(feature, struct7) * 10000.0 / 2650.0;
}

struct feature_benchmarker {
  benchmarker utf8;
  benchmarker utf8_miss;
  benchmarker empty;
  benchmarker empty_miss;
  benchmarker struct7;
  benchmarker struct7_miss;
  benchmarker struct7_full;
  benchmarker struct15;
  benchmarker struct15_miss;
  benchmarker struct23;
  benchmarker struct23_miss;

  feature_benchmarker(json_parser& parser, event_collector& collector) :
    utf8               ("jsonexamples/generated/utf-8.json", parser, collector),
    utf8_miss          ("jsonexamples/generated/utf-8-miss.json", parser, collector),
    empty              ("jsonexamples/generated/0-structurals.json", parser, collector),
    empty_miss         ("jsonexamples/generated/0-structurals-miss.json", parser, collector),
    struct7           ("jsonexamples/generated/7-structurals.json", parser, collector),
    struct7_miss      ("jsonexamples/generated/7-structurals-miss.json", parser, collector),
    struct7_full       ("jsonexamples/generated/7-structurals-full.json", parser, collector),
    struct15     ("jsonexamples/generated/15-structurals.json", parser, collector),
    struct15_miss("jsonexamples/generated/15-structurals-miss.json", parser, collector),
    struct23     ("jsonexamples/generated/23-structurals.json", parser, collector),
    struct23_miss("jsonexamples/generated/23-structurals-miss.json", parser, collector)
  {

  }

  really_inline void run_iterations(size_t iterations, bool stage1_only=false) {
    struct7.run_iterations(iterations, stage1_only);
    struct7_miss.run_iterations(iterations, stage1_only);
    struct7_full.run_iterations(iterations, stage1_only);
    utf8.run_iterations(iterations, stage1_only);
    utf8_miss.run_iterations(iterations, stage1_only);
    empty.run_iterations(iterations, stage1_only);
    empty_miss.run_iterations(iterations, stage1_only);
    struct15.run_iterations(iterations, stage1_only);
    struct15_miss.run_iterations(iterations, stage1_only);
    struct23.run_iterations(iterations, stage1_only);
    struct23_miss.run_iterations(iterations, stage1_only);
  }

  void print() {
    printf("base (ns/block)");
    printf(",struct 1-7");
    printf(",struct 1-7 miss");
    printf(",utf-8");
    printf(",utf-8 miss");
    printf(",struct 8-15");
    printf(",struct 8-15 miss");
    printf(",struct 16+");
    printf(",struct 16+ miss");
    printf("\n");

    printf("%g",   actual(empty));
    printf(",%+g", diff(struct7, empty));
    printf(",%+g", diff(struct7_miss, struct7));
    printf(",%+g", diff(utf8, struct7));
    printf(",%+g", diff(utf8_miss, utf8));
    printf(",%+g", diff(struct15, struct7));
    printf(",%+g", diff(struct15_miss, struct15));
    printf(",%+g", diff(struct23, struct15));
    printf(",%+g", diff(struct23_miss, struct23));
    printf("\n");
  }

  double cost_per_block(benchmarker& feature, size_t feature_blocks, benchmarker& base) {
    return (feature.stage1.best.elapsed_ns() - base.stage1.best.elapsed_ns()) / feature_blocks;
  }

  // Base cost of any block (including empty ones)
  double base_cost() {
    return (empty.stage1.best.elapsed_ns() / empty.stats->blocks);
  }
  // Extra cost of a 1-7 structural block over an empty block
  double struct1_7_cost() {
    return cost_per_block(struct7, struct7.stats->blocks_with_1_structural, empty);
  }
  // Extra cost of an 1-7-structural miss
  double struct1_7_miss_cost() {
    return cost_per_block(struct7_miss, struct7_miss.stats->blocks_with_1_structural, struct7);
  }
  // Extra cost of an 8-15 structural block over a 1-7 structural block
  double struct8_15_cost() {
    return cost_per_block(struct15, struct15.stats->blocks_with_8_structurals, struct7);
  }
  // Extra cost of an 8-15-structural miss over a 1-7 miss
  double struct8_15_miss_cost() {
    return cost_per_block(struct15_miss, struct15_miss.stats->blocks_with_8_structurals_flipped, struct15);
  }
  // Extra cost of a 16+-structural block over an 8-15 structural block (actual varies based on # of structurals!)
  double struct16_cost() {
    return cost_per_block(struct23, struct23.stats->blocks_with_16_structurals, struct15);
  }
  // Extra cost of a 16-structural miss over an 8-15 miss
  double struct16_miss_cost() {
    return cost_per_block(struct23_miss, struct23_miss.stats->blocks_with_16_structurals_flipped, struct23);
  }
  // Extra cost of having UTF-8 in a block
  double utf8_cost() {
    return cost_per_block(utf8, utf8.stats->blocks_with_utf8, struct7_full);
  }
  // Extra cost of a UTF-8 miss
  double utf8_miss_cost() {
    return cost_per_block(utf8_miss, utf8_miss.stats->blocks_with_utf8_flipped, utf8);
  }

  double calc_expected(benchmarker& file) {
    // Expected base ns/block (empty)
    json_stats& stats = *file.stats;
    double expected = base_cost()      * stats.blocks;
    expected += struct1_7_cost()       * stats.blocks_with_1_structural;
    expected += struct1_7_miss_cost()  * stats.blocks_with_1_structural_flipped;
    expected += utf8_cost()            * stats.blocks_with_utf8;
    expected += utf8_miss_cost()       * stats.blocks_with_utf8_flipped;
    expected += struct8_15_cost()      * stats.blocks_with_8_structurals;
    expected += struct8_15_miss_cost() * stats.blocks_with_8_structurals_flipped;
    expected += struct16_cost()        * stats.blocks_with_16_structurals;
    expected += struct16_miss_cost()   * stats.blocks_with_16_structurals_flipped;
    return expected / stats.blocks;
  }
};

int main(int argc, char *argv[]) {
  // Read options
  exe_name = argv[0];
  option_struct options(argc, argv);
  if (options.verbose) {
    verbose_stream = &cout;
  }

  // Initialize the event collector. We put this early so if it prints an error message, it's the
  // first thing printed.
  event_collector collector;

  // Set up benchmarkers by reading all files
  json_parser parser(options.architecture);

  feature_benchmarker features(parser, collector);
  benchmarker gsoc_2018("jsonexamples/gsoc-2018.json", parser, collector);
  benchmarker twitter("jsonexamples/twitter.json", parser, collector);
  benchmarker random("jsonexamples/random.json", parser, collector);

  // Run the benchmarks
  progress_bar progress(options.iterations, 100);
  // Put the if (options.stage1_only) *outside* the loop so that run_iterations will be optimized
  if (options.stage1_only) {
    for (int iteration = 0; iteration < options.iterations; iteration += options.iteration_step) {
      if (!options.verbose) { progress.print(iteration); }
      features.run_iterations(options.iteration_step, true);
      gsoc_2018.run_iterations(options.iteration_step, true);
      twitter.run_iterations(options.iteration_step, true);
      random.run_iterations(options.iteration_step, true);
    }
  } else {
    for (int iteration = 0; iteration < options.iterations; iteration += options.iteration_step) {
      if (!options.verbose) { progress.print(iteration); }
      features.run_iterations(options.iteration_step, false);
      gsoc_2018.run_iterations(options.iteration_step, false);
      twitter.run_iterations(options.iteration_step, false);
      random.run_iterations(options.iteration_step, false);
    }
  }
  if (!options.verbose) { progress.erase(); }

  features.print();

// Gauge effectiveness
  printf("gsoc-2018.json expected/actual: %g/%g\n", features.calc_expected(gsoc_2018), actual(gsoc_2018));
  printf("twitter.json expected/actual: %g/%g\n", features.calc_expected(twitter), actual(twitter));
  printf("random.json expected/actual: %g/%g\n", features.calc_expected(random), actual(random));

  return EXIT_SUCCESS;
}
