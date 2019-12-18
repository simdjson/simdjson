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

  double cost_per_block(const benchmarker& feature, size_t feature_blocks, const benchmarker& base) const {
    return (feature.stage1.best.elapsed_ns() - base.stage1.best.elapsed_ns()) / feature_blocks;
  }

  // Whether we're recording cache miss and branch miss events
  bool has_events() const {
    return empty.collector.has_events();
  }

  // Base cost of any block (including empty ones)
  double base_cost() const {
    return (empty.stage1.best.elapsed_ns() / empty.stats->blocks);
  }

  // Extra cost of a 1-7 structural block over an empty block
  double struct1_7_cost() const {
    return cost_per_block(struct7, struct7.stats->blocks_with_1_structural, empty);
  }
  // Extra cost of an 1-7-structural miss
  double struct1_7_miss_cost() const {
    return cost_per_block(struct7_miss, struct7_miss.stats->blocks_with_1_structural, struct7);
  }
  // Rate of 1-7-structural misses per 8-structural flip
  double struct1_7_miss_rate() const {
    if (!has_events()) { return 1; }
    return double(struct7_miss.stage1.best.branch_misses() - struct7.stage1.best.branch_misses()) / struct7_miss.stats->blocks_with_1_structural_flipped;
  }

  // Extra cost of an 8-15 structural block over a 1-7 structural block
  double struct8_15_cost() const {
    return cost_per_block(struct15, struct15.stats->blocks_with_8_structurals, struct7);
  }
  // Extra cost of an 8-15-structural miss over a 1-7 miss
  double struct8_15_miss_cost() const {
    return cost_per_block(struct15_miss, struct15_miss.stats->blocks_with_8_structurals_flipped, struct15);
  }
  // Rate of 8-15-structural misses per 8-structural flip
  double struct8_15_miss_rate() const {
    if (!has_events()) { return 1; }
    return double(struct15_miss.stage1.best.branch_misses() - struct15.stage1.best.branch_misses()) / struct15_miss.stats->blocks_with_8_structurals_flipped;
  }

  // Extra cost of a 16+-structural block over an 8-15 structural block (actual varies based on # of structurals!)
  double struct16_cost() const {
    return cost_per_block(struct23, struct23.stats->blocks_with_16_structurals, struct15);
  }
  // Extra cost of a 16-structural miss over an 8-15 miss
  double struct16_miss_cost() const {
    return cost_per_block(struct23_miss, struct23_miss.stats->blocks_with_16_structurals_flipped, struct23);
  }
  // Rate of 16-structural misses per 16-structural flip
  double struct16_miss_rate() const {
    if (!has_events()) { return 1; }
    return double(struct23_miss.stage1.best.branch_misses() - struct23.stage1.best.branch_misses()) / struct23_miss.stats->blocks_with_16_structurals_flipped;
  }

  // Extra cost of having UTF-8 in a block
  double utf8_cost() const {
    return cost_per_block(utf8, utf8.stats->blocks_with_utf8, struct7_full);
  }
  // Extra cost of a UTF-8 miss
  double utf8_miss_cost() const {
    return cost_per_block(utf8_miss, utf8_miss.stats->blocks_with_utf8_flipped, utf8);
  }
  // Rate of UTF-8 misses per UTF-8 flip
  double utf8_miss_rate() const {
    if (!has_events()) { return 1; }
    return double(utf8_miss.stage1.best.branch_misses() - utf8.stage1.best.branch_misses()) / utf8_miss.stats->blocks_with_utf8_flipped;
  }

  double calc_expected_feature_cost(const benchmarker& file) const {
    // Expected base ns/block (empty)
    json_stats& stats = *file.stats;
    double expected = base_cost()       * stats.blocks;
    expected +=       struct1_7_cost()  * stats.blocks_with_1_structural;
    expected +=       utf8_cost()       * stats.blocks_with_utf8;
    expected +=       struct8_15_cost() * stats.blocks_with_8_structurals;
    expected +=       struct16_cost()   * stats.blocks_with_16_structurals;
    return expected / stats.blocks;
  }

  double calc_expected_miss_cost(const benchmarker& file) const {
    // Expected base ns/block (empty)
    json_stats& stats = *file.stats;
    double expected = struct1_7_miss_cost()  * stats.blocks_with_1_structural_flipped * struct1_7_miss_rate();
    expected +=       utf8_miss_cost()       * stats.blocks_with_utf8_flipped * utf8_miss_rate();
    expected +=       struct8_15_miss_cost() * stats.blocks_with_8_structurals_flipped * struct8_15_miss_rate();
    expected +=       struct16_miss_cost()   * stats.blocks_with_16_structurals_flipped * struct16_miss_rate();
    return expected / stats.blocks;
  }

  double calc_expected_misses(const benchmarker& file) const {
    json_stats& stats = *file.stats;
    double expected = stats.blocks_with_1_structural_flipped   * struct1_7_miss_rate();
    expected +=       stats.blocks_with_utf8_flipped           * utf8_miss_rate();
    expected +=       stats.blocks_with_8_structurals_flipped  * struct8_15_miss_rate();
    expected +=       stats.blocks_with_16_structurals_flipped * struct16_miss_rate();
    return expected;
  }

  double calc_expected(const benchmarker& file) const {
    return calc_expected_feature_cost(file) + calc_expected_miss_cost(file);
  }

  void print() {
    printf("\n");
    printf("Features in ns/block (64 bytes):\n");
    printf("\n");
    printf("| %-8s ",   "Stage");
    printf("| %8s ",  "Base");
    printf("| %8s ",  "7 Struct");
    printf("| %8s ",  "UTF-8");
    printf("| %8s ",  "15 Str.");
    printf("| %8s ",  "16+ Str.");
    printf("| %15s ", "7 Struct Miss");
    printf("| %15s ", "UTF-8 Miss");
    printf("| %15s ", "15 Str. Miss");
    printf("| %15s ", "16+ Str. Miss");
    printf("|\n");

    printf("|%.10s",  "---------------------------------------");
    printf("|%.10s",  "---------------------------------------");
    printf("|%.10s",  "---------------------------------------");
    printf("|%.10s",  "---------------------------------------");
    printf("|%.10s",  "---------------------------------------");
    printf("|%.10s",  "---------------------------------------");
    printf("|%.17s", "---------------------------------------");
    printf("|%.17s", "---------------------------------------");
    printf("|%.17s", "---------------------------------------");
    printf("|%.17s", "---------------------------------------");
    printf("|\n");

    printf("| %-8s ",         "Stage 1");
    printf("| %8.3g ",        base_cost());
    printf("| %8.3g ",        struct1_7_cost());
    printf("| %8.3g ",        utf8_cost());
    printf("| %8.3g ",        struct8_15_cost());
    printf("| %8.3g ",        struct16_cost());
    if (has_events()) {
      printf("| %8.3g (%3d%%) ", struct1_7_miss_cost(), int(struct1_7_miss_rate()*100));
      printf("| %8.3g (%3d%%) ", utf8_miss_cost(), int(utf8_miss_rate()*100));
      printf("| %8.3g (%3d%%) ", struct8_15_miss_cost(), int(struct8_15_miss_rate()*100));
      printf("| %8.3g (%3d%%) ", struct16_miss_cost(), int(struct16_miss_rate()*100));
    } else {
      printf("|        %8.3g ", struct1_7_miss_cost());
      printf("|        %8.3g ", utf8_miss_cost());
      printf("|        %8.3g ", struct8_15_miss_cost());
      printf("|        %8.3g ", struct16_miss_cost());
    }
    printf("|\n");
  }
};

void print_file_effectiveness(const char* filename, const benchmarker& results, const feature_benchmarker& features) {
  double actual = results.stage1.best.elapsed_ns() / results.stats->blocks;
  double calc = features.calc_expected(results);
  uint64_t actual_misses = results.stage1.best.branch_misses();
  uint64_t calc_misses = uint64_t(features.calc_expected_misses(results));
  double calc_miss_cost = features.calc_expected_miss_cost(results);
  printf("| %-15s ",   filename);
  printf("|    %8.3g ", features.calc_expected_feature_cost(results));
  printf("|    %8.3g ", calc_miss_cost);
  printf("| %8.3g ",  calc);
  printf("| %8.3g ",  actual);
  printf("| %+8.3g ", actual - calc);
  printf("| %13lu ",  calc_misses);
  if (features.has_events()) {
    printf("| %13lu ",  actual_misses);
    printf("| %+13ld ", int64_t(actual_misses - calc_misses));
    double miss_adjustment = calc_miss_cost * (double(int64_t(actual_misses - calc_misses)) / calc_misses);
    printf("|      %8.3g ", calc_miss_cost + miss_adjustment);
    printf("|      %+8.3g ", actual - (calc + miss_adjustment));
  }
  printf("|\n");
}

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
  printf("\n");
  printf("Estimated vs. Actual ns/block for real files:\n");
  printf("\n");
  printf("| %-15s ", "File");
  printf("| %11s ", "Est. (Base)");
  printf("| %11s ", "Est. (Miss)");
  printf("| %8s ",  "Est.");
  printf("| %8s ",  "Actual");
  printf("| %8s ",  "Diff");
  printf("| %13s ", "Est. Misses");
  if (features.has_events()) {
    printf("| %13s ", "Actual Misses");
    printf("| %13s ", "Diff (Misses)");
    printf("| %13s ", "Adjusted Miss");
    printf("| %13s ", "Adjusted Diff");
  }
  printf("|\n");
  printf("|%.17s",  "---------------------------------------");
  printf("|%.13s",  "---------------------------------------");
  printf("|%.13s",  "---------------------------------------");
  printf("|%.10s",  "---------------------------------------");
  printf("|%.10s",  "---------------------------------------");
  printf("|%.10s",  "---------------------------------------");
  printf("|%.15s",  "---------------------------------------");
  if (features.has_events()) {
    printf("|%.15s",  "---------------------------------------");
    printf("|%.15s",  "---------------------------------------");
    printf("|%.15s",  "---------------------------------------");
    printf("|%.15s",  "---------------------------------------");
  }
  printf("|\n");

  print_file_effectiveness("gsoc-2018.json", gsoc_2018, features);
  print_file_effectiveness("twitter.json", twitter, features);
  print_file_effectiveness("random.json", random, features);

  return EXIT_SUCCESS;
}
