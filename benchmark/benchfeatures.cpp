#include "event_counter.h"

#include <cassert>
#include <cctype>
#ifndef _MSC_VER
#include <dirent.h>
#endif
#include <unistd.h>
#include <cinttypes>
#include <initializer_list>

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

#include "simdjson.h"

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
  out << "-a ARCH    - Use the parser with the designated architecture (HASWELL, WESTMERE," << endl;
  out << "             PPC64 or ARM64). By default, detects best supported architecture." << endl;
}

void exit_usage(string message) {
  cerr << message << endl;
  cerr << endl;
  print_usage(cerr);
  exit(EXIT_FAILURE);
}

struct option_struct {
  bool stage1_only = false;

  int32_t iterations = 400;
  int32_t iteration_step = 50;

  bool verbose = false;

  option_struct(int argc, char **argv) {
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
      case 'a': {
          auto impl = simdjson::get_available_implementations()[optarg];
          if(impl && impl->supported_by_runtime_system()) {
            simdjson::get_active_implementation() = impl;
          } else {
            std::cerr << "implementation " << optarg << " not found or not supported " << std::endl;
          }
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
        exit_error(string("Unexpected argument ") + std::string(1,static_cast<char>(c)));
      }
    }
  }

  template<typename F>
  void each_stage(const F& f) const {
    f(BenchmarkStage::STAGE1);
    if (!this->stage1_only) {
      f(BenchmarkStage::STAGE2);
      f(BenchmarkStage::ALL);
    }
  }

};

struct feature_benchmarker {
  benchmarker utf8;
  benchmarker utf8_miss;
  benchmarker escape;
  benchmarker escape_miss;
  benchmarker empty;
  benchmarker empty_miss;
  benchmarker struct7;
  benchmarker struct7_miss;
  benchmarker struct7_full;
  benchmarker struct15;
  benchmarker struct15_miss;
  benchmarker struct23;
  benchmarker struct23_miss;

  feature_benchmarker(event_collector& collector) :
    utf8               (SIMDJSON_BENCHMARK_DATA_DIR "generated/utf-8.json", collector),
    utf8_miss          (SIMDJSON_BENCHMARK_DATA_DIR "generated/utf-8-miss.json", collector),
    escape               (SIMDJSON_BENCHMARK_DATA_DIR "generated/escape.json", collector),
    escape_miss          (SIMDJSON_BENCHMARK_DATA_DIR "generated/escape-miss.json", collector),
    empty              (SIMDJSON_BENCHMARK_DATA_DIR "generated/0-structurals.json", collector),
    empty_miss         (SIMDJSON_BENCHMARK_DATA_DIR "generated/0-structurals-miss.json", collector),
    struct7           (SIMDJSON_BENCHMARK_DATA_DIR "generated/7-structurals.json", collector),
    struct7_miss      (SIMDJSON_BENCHMARK_DATA_DIR "generated/7-structurals-miss.json", collector),
    struct7_full       (SIMDJSON_BENCHMARK_DATA_DIR "generated/7-structurals-full.json", collector),
    struct15     (SIMDJSON_BENCHMARK_DATA_DIR "generated/15-structurals.json", collector),
    struct15_miss(SIMDJSON_BENCHMARK_DATA_DIR "generated/15-structurals-miss.json", collector),
    struct23     (SIMDJSON_BENCHMARK_DATA_DIR "generated/23-structurals.json", collector),
    struct23_miss(SIMDJSON_BENCHMARK_DATA_DIR "generated/23-structurals-miss.json", collector)
  {

  }

  simdjson_inline void run_iterations(size_t iterations, bool stage1_only=false) {
    struct7.run_iterations(iterations, stage1_only);
    struct7_miss.run_iterations(iterations, stage1_only);
    struct7_full.run_iterations(iterations, stage1_only);
    utf8.run_iterations(iterations, stage1_only);
    utf8_miss.run_iterations(iterations, stage1_only);
    escape.run_iterations(iterations, stage1_only);
    escape_miss.run_iterations(iterations, stage1_only);
    empty.run_iterations(iterations, stage1_only);
    empty_miss.run_iterations(iterations, stage1_only);
    struct15.run_iterations(iterations, stage1_only);
    struct15_miss.run_iterations(iterations, stage1_only);
    struct23.run_iterations(iterations, stage1_only);
    struct23_miss.run_iterations(iterations, stage1_only);
  }

  double cost_per_block(BenchmarkStage stage, const benchmarker& feature, size_t feature_blocks, const benchmarker& base) const {
    return (feature[stage].best.elapsed_ns() - base[stage].best.elapsed_ns()) / double(feature_blocks);
  }

  // Whether we're recording cache miss and branch miss events
  bool has_events() const {
    return empty.collector.has_events();
  }

  // Base cost of any block (including empty ones)
  double base_cost(BenchmarkStage stage) const {
    return (empty[stage].best.elapsed_ns() / double(empty.stats->blocks));
  }

  // Extra cost of a 1-7 structural block over an empty block
  double struct1_7_cost(BenchmarkStage stage) const {
    return cost_per_block(stage, struct7, struct7.stats->blocks_with_1_structural, empty);
  }
  // Extra cost of an 1-7-structural miss
  double struct1_7_miss_cost(BenchmarkStage stage) const {
    return cost_per_block(stage, struct7_miss, struct7_miss.stats->blocks_with_1_structural, struct7);
  }
  // Rate of 1-7-structural misses per 8-structural flip
  double struct1_7_miss_rate(BenchmarkStage stage) const {
#if SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
    return 1;
#else
    if (!has_events()) { return 1; }
    return struct7_miss[stage].best.branch_misses() - struct7[stage].best.branch_misses() / double(struct7_miss.stats->blocks_with_1_structural_flipped);
#endif
  }
  // Extra cost of an 8-15 structural block over a 1-7 structural block
  double struct8_15_cost(BenchmarkStage stage) const {
    return cost_per_block(stage, struct15, struct15.stats->blocks_with_8_structurals, struct7);
  }
  // Extra cost of an 8-15-structural miss over a 1-7 miss
  double struct8_15_miss_cost(BenchmarkStage stage) const {
    return cost_per_block(stage, struct15_miss, struct15_miss.stats->blocks_with_8_structurals_flipped, struct15);
  }
  // Rate of 8-15-structural misses per 8-structural flip
  double struct8_15_miss_rate(BenchmarkStage stage) const {
#if SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
    return 1;
#else
    if (!has_events()) { return 1; }
    return double(struct15_miss[stage].best.branch_misses() - struct15[stage].best.branch_misses()) / double(struct15_miss.stats->blocks_with_8_structurals_flipped);
#endif
  }

  // Extra cost of a 16+-structural block over an 8-15 structural block (actual varies based on # of structurals!)
  double struct16_cost(BenchmarkStage stage) const {
    return cost_per_block(stage, struct23, struct23.stats->blocks_with_16_structurals, struct15);
  }
  // Extra cost of a 16-structural miss over an 8-15 miss
  double struct16_miss_cost(BenchmarkStage stage) const {
    return cost_per_block(stage, struct23_miss, struct23_miss.stats->blocks_with_16_structurals_flipped, struct23);
  }
  // Rate of 16-structural misses per 16-structural flip
  double struct16_miss_rate(BenchmarkStage stage) const {
#if SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
    return 1;
#else
    if (!has_events()) { return 1; }
    return double(struct23_miss[stage].best.branch_misses() - struct23[stage].best.branch_misses()) / double(struct23_miss.stats->blocks_with_16_structurals_flipped);
#endif
  }


  // Extra cost of having UTF-8 in a block
  double utf8_cost(BenchmarkStage stage) const {
    return cost_per_block(stage, utf8, utf8.stats->blocks_with_utf8, struct7_full);
  }
  // Extra cost of a UTF-8 miss
  double utf8_miss_cost(BenchmarkStage stage) const {
    return cost_per_block(stage, utf8_miss, utf8_miss.stats->blocks_with_utf8_flipped, utf8);
  }
  // Rate of UTF-8 misses per UTF-8 flip
  double utf8_miss_rate(BenchmarkStage stage) const {
#if SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
    return 1;
#else
    if (!has_events()) { return 1; }
    return double(utf8_miss[stage].best.branch_misses() - utf8[stage].best.branch_misses()) / double(utf8_miss.stats->blocks_with_utf8_flipped);
#endif
  }
  // Extra cost of having escapes in a block
  double escape_cost(BenchmarkStage stage) const {
    return cost_per_block(stage, escape, escape.stats->blocks_with_escapes, struct7_full);
  }
  // Extra cost of an escape miss
  double escape_miss_cost(BenchmarkStage stage) const {
    return cost_per_block(stage, escape_miss, escape_miss.stats->blocks_with_escapes_flipped, escape);
  }
  // Rate of escape misses per escape flip
  double escape_miss_rate(BenchmarkStage stage) const {
#if SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
    return 1;
#else
    if (!has_events()) { return 1; }
    return double(escape_miss[stage].best.branch_misses() - escape[stage].best.branch_misses()) / double(escape_miss.stats->blocks_with_escapes_flipped);
#endif
  }


  double calc_expected_feature_cost(BenchmarkStage stage, const benchmarker& file) const {
    // Expected base ns/block (empty)
    json_stats& stats = *file.stats;
    double expected = base_cost(stage)       * double(stats.blocks);
    expected +=       struct1_7_cost(stage)  * double(stats.blocks_with_1_structural);
    expected +=       utf8_cost(stage)       * double(stats.blocks_with_utf8);
    expected +=       escape_cost(stage)     * double(stats.blocks_with_escapes);
    expected +=       struct8_15_cost(stage) * double(stats.blocks_with_8_structurals);
    expected +=       struct16_cost(stage)   * double(stats.blocks_with_16_structurals);
    return expected / double(stats.blocks);
  }

  double calc_expected_miss_cost(BenchmarkStage stage, const benchmarker& file) const {
    // Expected base ns/block (empty)
    json_stats& stats = *file.stats;
    double expected = struct1_7_miss_cost(stage)  * double(stats.blocks_with_1_structural_flipped) * struct1_7_miss_rate(stage);
    expected +=       utf8_miss_cost(stage)       * double(stats.blocks_with_utf8_flipped) * utf8_miss_rate(stage);
    expected +=       escape_miss_cost(stage)     * double(stats.blocks_with_escapes_flipped) * escape_miss_rate(stage);
    expected +=       struct8_15_miss_cost(stage) * double(stats.blocks_with_8_structurals_flipped) * struct8_15_miss_rate(stage);
    expected +=       struct16_miss_cost(stage)   * double(stats.blocks_with_16_structurals_flipped) * struct16_miss_rate(stage);
    return expected / double(stats.blocks);
  }

  double calc_expected_misses(BenchmarkStage stage, const benchmarker& file) const {
    json_stats& stats = *file.stats;
    double expected = double(stats.blocks_with_1_structural_flipped)   * struct1_7_miss_rate(stage);
    expected +=       double(stats.blocks_with_utf8_flipped)           * utf8_miss_rate(stage);
    expected +=       double(stats.blocks_with_escapes_flipped)        * escape_miss_rate(stage);
    expected +=       double(stats.blocks_with_8_structurals_flipped)  * struct8_15_miss_rate(stage);
    expected +=       double(stats.blocks_with_16_structurals_flipped) * struct16_miss_rate(stage);
    return expected;
  }

  double calc_expected(BenchmarkStage stage, const benchmarker& file) const {
    return calc_expected_feature_cost(stage, file) + calc_expected_miss_cost(stage, file);
  }
  void print(const option_struct& options) const {
    printf("\n");
    printf("Features in ns/block (64 bytes):\n");
    printf("\n");
    printf("| %-8s ",   "Stage");
    printf("| %8s ",  "Base");
    printf("| %8s ",  "7 Struct");
    printf("| %8s ",  "UTF-8");
    printf("| %8s ",  "Escape");
    printf("| %8s ",  "15 Str.");
    printf("| %8s ",  "16+ Str.");
    printf("| %15s ", "7 Struct Miss");
    printf("| %15s ", "UTF-8 Miss");
    printf("| %15s ", "Escape Miss");
    printf("| %15s ", "15 Str. Miss");
    printf("| %15s ", "16+ Str. Miss");
    printf("|\n");

    printf("|%.10s",  "---------------------------------------");
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
    printf("|%.17s", "---------------------------------------");
    printf("|\n");

    options.each_stage([&](auto stage) {
      printf("| %-8s ",         benchmark_stage_name(stage));
      printf("| %8.3g ",        base_cost(stage));
      printf("| %8.3g ",        struct1_7_cost(stage));
      printf("| %8.3g ",        utf8_cost(stage));
      printf("| %8.3g ",        escape_cost(stage));
      printf("| %8.3g ",        struct8_15_cost(stage));
      printf("| %8.3g ",        struct16_cost(stage));
      if (has_events()) {
        printf("| %8.3g (%3d%%) ", struct1_7_miss_cost(stage), int(struct1_7_miss_rate(stage)*100));
        printf("| %8.3g (%3d%%) ", utf8_miss_cost(stage), int(utf8_miss_rate(stage)*100));
        printf("| %8.3g (%3d%%) ", escape_miss_cost(stage), int(escape_miss_rate(stage)*100));
        printf("| %8.3g (%3d%%) ", struct8_15_miss_cost(stage), int(struct8_15_miss_rate(stage)*100));
        printf("| %8.3g (%3d%%) ", struct16_miss_cost(stage), int(struct16_miss_rate(stage)*100));
      } else {
        printf("|        %8.3g ", struct1_7_miss_cost(stage));
        printf("|        %8.3g ", utf8_miss_cost(stage));
        printf("|        %8.3g ", escape_miss_cost(stage));
        printf("|        %8.3g ", struct8_15_miss_cost(stage));
        printf("|        %8.3g ", struct16_miss_cost(stage));
      }
      printf("|\n");
    });
  }
};

#if SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
void print_file_effectiveness(BenchmarkStage stage, const char* filename, const benchmarker& results, const feature_benchmarker& features) {
  double actual = results[stage].best.elapsed_ns() / double(results.stats->blocks);
  double calc = features.calc_expected(stage, results);
  double calc_misses = features.calc_expected_misses(stage, results);
  double calc_miss_cost = features.calc_expected_miss_cost(stage, results);
  printf("        | %-8s ", benchmark_stage_name(stage));
  printf("| %-15s ",   filename);
  printf("|    %8.3g ", features.calc_expected_feature_cost(stage, results));
  printf("|    %8.3g ", calc_miss_cost);
  printf("| %8.3g ",  calc);
  printf("| %8.3g ",  actual);
  printf("| %+8.3g ", actual - calc);
  printf("| %13llu ", (long long unsigned)(calc_misses));
}
#else
void print_file_effectiveness(BenchmarkStage stage, const char* filename, const benchmarker& results, const feature_benchmarker& features) {
  double actual = results[stage].best.elapsed_ns() / double(results.stats->blocks);
  double calc = features.calc_expected(stage, results);
  double actual_misses = results[stage].best.branch_misses();
  double calc_misses = features.calc_expected_misses(stage, results);
  double calc_miss_cost = features.calc_expected_miss_cost(stage, results);
  printf("        | %-8s ", benchmark_stage_name(stage));
  printf("| %-15s ",   filename);
  printf("|    %8.3g ", features.calc_expected_feature_cost(stage, results));
  printf("|    %8.3g ", calc_miss_cost);
  printf("| %8.3g ",  calc);
  printf("| %8.3g ",  actual);
  printf("| %+8.3g ", actual - calc);
  printf("| %13llu ", (long long unsigned)(calc_misses));
  if (features.has_events()) {
    printf("| %13llu ", (long long unsigned)(actual_misses));
    printf("| %+13lld ", (long long int)(actual_misses - calc_misses));
    double miss_adjustment = calc_miss_cost * (double(int64_t(actual_misses - calc_misses)) / calc_misses);
    printf("|      %8.3g ", calc_miss_cost + miss_adjustment);
    printf("|      %+8.3g ", actual - (calc + miss_adjustment));
  }
  printf("|\n");
}
#endif

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
  feature_benchmarker features(collector);
  benchmarker gsoc_2018(SIMDJSON_BENCHMARK_DATA_DIR "gsoc-2018.json", collector);
  benchmarker twitter(SIMDJSON_BENCHMARK_DATA_DIR "twitter.json", collector);
  benchmarker random(SIMDJSON_BENCHMARK_DATA_DIR "random.json", collector);

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

  features.print(options);

  // Gauge effectiveness
  if (options.verbose) {
    printf("\n");
    printf("        Effectiveness Check: Estimated vs. Actual ns/block for real files:\n");
    printf("\n");
    printf("        | %8s ", "Stage");
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
    printf("        |%.10s",  "---------------------------------------");
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

    options.each_stage([&](auto stage) {
      print_file_effectiveness(stage, "gsoc-2018.json", gsoc_2018, features);
      print_file_effectiveness(stage, "twitter.json", twitter, features);
      print_file_effectiveness(stage, "random.json", random, features);
    });
  }

  return EXIT_SUCCESS;
}
