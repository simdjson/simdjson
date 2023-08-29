#ifndef __BENCHMARKER_H
#define __BENCHMARKER_H

#include "event_counter.h"
#include "simdjson.h"

#include <cassert>
#include <cctype>
#ifndef _MSC_VER
#include <dirent.h>
#endif
#include <unistd.h>
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
#include "simdjson.h"

#include <functional>

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
using std::min;
using std::max;

// Initialize "verbose" to go nowhere. We'll read options in main() and set to cout if verbose is true.
std::ofstream dev_null;
ostream *verbose_stream = &dev_null;
const size_t BYTES_PER_BLOCK = 64;

ostream& verbose() {
  return *verbose_stream;
}

void exit_error(string message) {
  cerr << message << endl;
  exit(EXIT_FAILURE);
}

struct json_stats {
  size_t bytes = 0;
  size_t blocks = 0;
  size_t structurals = 0;
  size_t blocks_with_utf8 = 0;
  size_t blocks_with_utf8_flipped = 0;
  size_t blocks_with_escapes = 0;
  size_t blocks_with_escapes_flipped = 0;
  size_t blocks_with_0_structurals = 0;
  size_t blocks_with_0_structurals_flipped = 0;
  size_t blocks_with_1_structural = 0;
  size_t blocks_with_1_structural_flipped = 0;
  size_t blocks_with_8_structurals = 0;
  size_t blocks_with_8_structurals_flipped = 0;
  size_t blocks_with_16_structurals = 0;
  size_t blocks_with_16_structurals_flipped = 0;

  json_stats(const padded_string& json, const dom::parser& parser) {
    bytes = json.size();
    blocks = bytes / BYTES_PER_BLOCK;
    if (bytes % BYTES_PER_BLOCK > 0) { blocks++; } // Account for remainder block
    structurals = parser.implementation->n_structural_indexes-1;

    // Calculate stats on blocks that will trigger utf-8 if statements / mispredictions
    bool last_block_has_utf8 = false;
    for (size_t block=0; block<blocks; block++) {
      // Find utf-8 in the block
      size_t block_start = block*BYTES_PER_BLOCK;
      size_t block_end = block_start+BYTES_PER_BLOCK;
      if (block_end > json.size()) { block_end = json.size(); }
      bool block_has_utf8 = false;
      for (size_t i=block_start; i<block_end; i++) {
        if (json.data()[i] & 0x80) {
          block_has_utf8 = true;
          break;
        }
      }
      if (block_has_utf8) {
        blocks_with_utf8++;
      }
      if (block > 0 && last_block_has_utf8 != block_has_utf8) {
        blocks_with_utf8_flipped++;
      }
      last_block_has_utf8 = block_has_utf8;
    }

    // Calculate stats on blocks that will trigger escape if statements / mispredictions
    bool last_block_has_escapes = false;
    for (size_t block=0; block<blocks; block++) {
      // Find utf-8 in the block
      size_t block_start = block*BYTES_PER_BLOCK;
      size_t block_end = block_start+BYTES_PER_BLOCK;
      if (block_end > json.size()) { block_end = json.size(); }
      bool block_has_escapes = false;
      for (size_t i=block_start; i<block_end; i++) {
        if (json.data()[i] == '\\') {
          block_has_escapes = true;
          break;
        }
      }
      if (block_has_escapes) {
        blocks_with_escapes++;
      }
      if (block > 0 && last_block_has_escapes != block_has_escapes) {
        blocks_with_escapes_flipped++;
      }
      last_block_has_escapes = block_has_escapes;
    }

    // Calculate stats on blocks that will trigger structural count if statements / mispredictions
    bool last_block_has_0_structurals = false;
    bool last_block_has_1_structural = false;
    bool last_block_has_8_structurals = false;
    bool last_block_has_16_structurals = false;
    size_t structural=0;
    for (size_t block=0; block<blocks; block++) {
      // Count structurals in the block
      int block_structurals=0;
      while (structural < parser.implementation->n_structural_indexes && parser.implementation->structural_indexes[structural] < (block+1)*BYTES_PER_BLOCK) {
        block_structurals++;
        structural++;
      }

      bool block_has_0_structurals = block_structurals == 0;
      if (block_has_0_structurals) {
        blocks_with_0_structurals++;
      }
      if (block > 0 && last_block_has_0_structurals != block_has_0_structurals) {
        blocks_with_0_structurals_flipped++;
      }
      last_block_has_0_structurals = block_has_0_structurals;

      bool block_has_1_structural = block_structurals >= 1;
      if (block_has_1_structural) {
        blocks_with_1_structural++;
      }
      if (block > 0 && last_block_has_1_structural != block_has_1_structural) {
        blocks_with_1_structural_flipped++;
      }
      last_block_has_1_structural = block_has_1_structural;

      bool block_has_8_structurals = block_structurals >= 8;
      if (block_has_8_structurals) {
        blocks_with_8_structurals++;
      }
      if (block > 0 && last_block_has_8_structurals != block_has_8_structurals) {
        blocks_with_8_structurals_flipped++;
      }
      last_block_has_8_structurals = block_has_8_structurals;

      bool block_has_16_structurals = block_structurals >= 16;
      if (block_has_16_structurals) {
        blocks_with_16_structurals++;
      }
      if (block > 0 && last_block_has_16_structurals != block_has_16_structurals) {
        blocks_with_16_structurals_flipped++;
      }
      last_block_has_16_structurals = block_has_16_structurals;
    }
  }
};

struct progress_bar {
  int max_value;
  int total_ticks;
  double ticks_per_value;
  int next_tick;
  progress_bar(int _max_value, int _total_ticks) : max_value(_max_value), total_ticks(_total_ticks), ticks_per_value(double(_total_ticks)/_max_value), next_tick(0) {
    fprintf(stderr, "[");
    for (int i=0;i<total_ticks;i++) {
      fprintf(stderr, " ");
    }
    fprintf(stderr, "]");
    for (int i=0;i<total_ticks+1;i++) {
      fprintf(stderr, "\b");
    }
  }

  void print(int value) {
    double ticks = value*ticks_per_value;
    if (ticks >= total_ticks) {
      ticks = total_ticks-1;
    }
    int tick;
    for (tick=next_tick; tick <= ticks && tick <= total_ticks; tick++) {
      fprintf(stderr, "=");
    }
    next_tick = tick;
  }
  void erase() const {
    for (int i=0;i<next_tick+1;i++) {
      fprintf(stderr, "\b");
    }
    for (int tick=0; tick<=total_ticks+2; tick++) {
      fprintf(stderr, " ");
    }
    for (int tick=0; tick<=total_ticks+2; tick++) {
      fprintf(stderr, "\b");
    }
  }
};

/**
 * The speed at which we can allocate memory is strictly system specific.
 * It depends on the OS and the runtime library. It is subject to various
 * system-specific knobs. It is not something that we can reasonably
 * benchmark with crude timings.
 * If someone wants to optimize how simdjson allocate memory, then it will
 * almost surely require a distinct benchmarking tool. What is meant by
 * "memory allocation" also requires a definition. Doing "new char[size]" can
 * do many different things depending on the system.
 */

enum class BenchmarkStage {
  ALL, // This excludes allocation
  ALLOCATE,
  STAGE1,
  STAGE2
};

const char* benchmark_stage_name(BenchmarkStage stage) {
  switch (stage) {
    case BenchmarkStage::ALL: return "All (Without Allocation)";
    case BenchmarkStage::ALLOCATE: return "Allocate";
    case BenchmarkStage::STAGE1: return "Stage 1";
    case BenchmarkStage::STAGE2: return "Stage 2";
    default: return "Unknown";
  }
}

struct benchmarker {
  // JSON text from loading the file. Owns the memory.
  padded_string json{};
  // JSON filename
  const char *filename;
  // Event collector that can be turned on to measure cycles, missed branches, etc.
  event_collector& collector;

  // Statistics about the JSON file independent of its speed (amount of utf-8, structurals, etc.).
  // Loaded on first parse.
  json_stats* stats;
  // Speed and event summary for full parse (stage 1 and stage 2, but *excluding* allocation)
  event_aggregate all_stages_without_allocation{};
  // Speed and event summary for stage 1
  event_aggregate stage1{};
  // Speed and event summary for stage 2
  event_aggregate stage2{};
  // Speed and event summary for allocation
  event_aggregate allocate_stage{};
  // Speed and event summary for the repeatly-parsing mode
  event_aggregate loop{};

  benchmarker(const char *_filename, event_collector& _collector)
    : filename(_filename), collector(_collector), stats(NULL) {
    verbose() << "[verbose] loading " << filename << endl;
    auto error = padded_string::load(filename).get(json);
    if (error) {
      exit_error(string("Could not load the file ") + filename);
    }
    verbose() << "[verbose] loaded " << filename << endl;
  }

  ~benchmarker() {
    if (stats) {
      delete stats;
    }
  }

  benchmarker(const benchmarker&) = delete;
  benchmarker& operator=(const benchmarker&) = delete;

  const event_aggregate& operator[](BenchmarkStage stage) const {
    switch (stage) {
      case BenchmarkStage::ALL: return this->all_stages_without_allocation;
      case BenchmarkStage::STAGE1: return this->stage1;
      case BenchmarkStage::STAGE2: return this->stage2;
      case BenchmarkStage::ALLOCATE: return this->allocate_stage;
      default: exit_error("Unknown stage"); return this->all_stages_without_allocation;
    }
  }

  int iterations() const {
    return all_stages_without_allocation.iterations;
  }

  simdjson_inline void run_iteration(bool stage1_only, bool hotbuffers=false) {
    // Allocate dom::parser
    collector.start();
    dom::parser parser;
    // We always allocate at least 64KB. Smaller allocations may actually be slower under some systems.
    error_code error = parser.allocate(json.size() < 65536 ? 65536 : json.size());
    if (error) {
      exit_error(string("Unable to allocate_stage ") + to_string(json.size()) + " bytes for the JSON text: " + error_message(error));
    }
    event_count allocate_count = collector.end();
    allocate_stage << allocate_count;
    // Run it once to get hot buffers
    if(hotbuffers) {
      auto result = parser.parse(reinterpret_cast<const uint8_t *>(json.data()), json.size());
      if (result.error()) {
        exit_error(string("Failed to parse ") + filename + string(":") + error_message(result.error()));
      }
    }

    verbose() << "[verbose] allocated memory for parsed JSON " << endl;

    // Stage 1 (find structurals)
    collector.start();
    error = parser.implementation->stage1(reinterpret_cast<const uint8_t *>(json.data()), json.size(), stage1_mode::regular);
    event_count stage1_count = collector.end();
    stage1 << stage1_count;
    if (error) {
      exit_error(string("Failed to parse ") + filename + " during stage 1: " + error_message(error));
    }

    // Stage 2 (unified machine) and the rest

    if (stage1_only) {
      all_stages_without_allocation << stage1_count;
    } else {
      event_count stage2_count;
      collector.start();
      error = parser.implementation->stage2(parser.doc);
      if (error) {
        exit_error(string("Failed to parse ") + filename + " during stage 2 parsing " + error_message(error));
      }
      stage2_count = collector.end();
      stage2 << stage2_count;
      all_stages_without_allocation << stage1_count + stage2_count;
    }
    // Calculate stats the first time we parse
    if (stats == NULL) {
      if (stage1_only) { //  we need stage 2 once
        error = parser.implementation->stage2(parser.doc);
        if (error) {
          printf("Warning: failed to parse during stage 2. Unable to acquire statistics.\n");
        }
      }
      stats = new json_stats(json, parser);
    }
  }

  void run_loop(size_t iterations) {
    dom::parser parser;
    auto firstresult = parser.parse(reinterpret_cast<const uint8_t *>(json.data()), json.size());
    if (firstresult.error()) {
      exit_error(string("Failed to parse ") + filename + string(":") + error_message(firstresult.error()));
    }

    collector.start();
    // some users want something closer to "number of documents per second"
    for(size_t i = 0; i < iterations; i++) {
      auto result = parser.parse(reinterpret_cast<const uint8_t *>(json.data()), json.size());
      if (result.error()) {
        exit_error(string("Failed to parse ") + filename + string(":") + error_message(result.error()));
      }
    }
    event_count all_loop_count = collector.end();
    loop << all_loop_count;
  }

  simdjson_inline void run_iterations(size_t iterations, bool stage1_only, bool hotbuffers=false) {
    for (size_t i = 0; i<iterations; i++) {
      run_iteration(stage1_only, hotbuffers);
    }
    run_loop(iterations);
  }

  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  template<typename T>
  void print_aggregate(const char* prefix, const T& stage) const {
    printf("%s%-13s: %8.4f ns per block (%6.2f%%) - %8.4f ns per byte - %8.4f ns per structural - %8.4f GB/s\n",
      prefix,
      "Speed",
      stage.elapsed_ns() / static_cast<double>(stats->blocks), // per block
      percent(stage.elapsed_sec(), all_stages_without_allocation.elapsed_sec()), // %
      stage.elapsed_ns() / static_cast<double>(stats->bytes), // per byte
      stage.elapsed_ns() / static_cast<double>(stats->structurals), // per structural
      (static_cast<double>(json.size()) / 1000000000.0) / stage.elapsed_sec() // GB/s
    );

    if (collector.has_events()) {
      printf("%s%-13s: %8.4f per block    (%6.2f%%) - %8.4f per byte    - %8.4f per structural    - %8.3f GHz est. frequency\n",
        prefix,
        "Cycles",
        stage.cycles() / static_cast<double>(stats->blocks),
        percent(stage.cycles(), all_stages_without_allocation.cycles()),
        stage.cycles() / static_cast<double>(stats->bytes),
        stage.cycles() / static_cast<double>(stats->structurals),
        (stage.cycles() / stage.elapsed_sec()) / 1000000000.0
      );
      printf("%s%-13s: %8.4f per block    (%6.2f%%) - %8.4f per byte    - %8.4f per structural    - %8.3f per cycle\n",
        prefix,
        "Instructions",
        stage.instructions() / static_cast<double>(stats->blocks),
        percent(stage.instructions(), all_stages_without_allocation.instructions()),
        stage.instructions() / static_cast<double>(stats->bytes),
        stage.instructions() / static_cast<double>(stats->structurals),
        stage.instructions() / static_cast<double>(stage.cycles())
      );
#if !SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
      // NOTE: removed cycles/miss because it is a somewhat misleading stat
      printf("%s%-13s: %7.0f branch misses (%6.2f%%) - %.0f cache misses (%6.2f%%) - %.2f cache references\n",
        prefix,
        "Misses",
        stage.branch_misses(),
        percent(stage.branch_misses(), all_stages_without_allocation.branch_misses()),
        stage.cache_misses(),
        percent(stage.cache_misses(), all_stages_without_allocation.cache_misses()),
        stage.cache_references()
      );
#endif
    }
  }

  static double percent(size_t a, size_t b) {
    return 100.0 * static_cast<double>(a) / static_cast<double>(b);
  }
  static double percent(double a, double b) {
    return 100.0 * a / b;
  }

  void print(bool tabbed_output, bool stage1_only) const {
    if (tabbed_output) {
      char* filename_copy = reinterpret_cast<char*>(malloc(strlen(filename)+1));
      SIMDJSON_PUSH_DISABLE_WARNINGS
      SIMDJSON_DISABLE_DEPRECATED_WARNING // Validated CRT_SECURE safe here
      strcpy(filename_copy, filename);
      SIMDJSON_POP_DISABLE_WARNINGS

      #if defined(__linux__)
      char* base = ::basename(filename_copy);
      #else
      char* base = filename_copy;
      #endif
      if (strlen(base) >= 5 && !strcmp(base+strlen(base)-5, ".json")) {
        base[strlen(base)-5] = '\0';
      }

      double gb = static_cast<double>(json.size()) / 1000000000.0;
      if (collector.has_events()) {
        printf("\"%s\"\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
                base,
                allocate_stage.best.cycles() / static_cast<double>(json.size()),
                stage1.best.cycles() / static_cast<double>(json.size()),
                stage2.best.cycles() / static_cast<double>(json.size()),
                all_stages_without_allocation.best.cycles() / static_cast<double>(json.size()),
                gb / all_stages_without_allocation.best.elapsed_sec(),
                gb / stage1.best.elapsed_sec(),
                gb / stage2.best.elapsed_sec());
      } else {
        printf("\"%s\"\t\t\t\t\t%f\t%f\t%f\n",
                base,
                gb / all_stages_without_allocation.best.elapsed_sec(),
                gb / stage1.best.elapsed_sec(),
                gb / stage2.best.elapsed_sec());
      }
      free(filename_copy);
    } else {
      printf("\n");
      printf("%s\n", filename);
      printf("%s\n", string(strlen(filename), '=').c_str());
      printf("%9zu blocks - %10zu bytes - %5zu structurals (%5.1f %%)\n", stats->bytes / BYTES_PER_BLOCK, stats->bytes, stats->structurals, percent(stats->structurals, stats->bytes));
      if (stats) {
        printf("special blocks with: utf8 %9zu (%5.1f %%) - escape %9zu (%5.1f %%) - 0 structurals %9zu (%5.1f %%) - 1+ structurals %9zu (%5.1f %%) - 8+ structurals %9zu (%5.1f %%) - 16+ structurals %9zu (%5.1f %%)\n",
          stats->blocks_with_utf8, percent(stats->blocks_with_utf8, stats->blocks),
          stats->blocks_with_escapes, percent(stats->blocks_with_escapes, stats->blocks),
          stats->blocks_with_0_structurals, percent(stats->blocks_with_0_structurals, stats->blocks),
          stats->blocks_with_1_structural, percent(stats->blocks_with_1_structural, stats->blocks),
          stats->blocks_with_8_structurals, percent(stats->blocks_with_8_structurals, stats->blocks),
          stats->blocks_with_16_structurals, percent(stats->blocks_with_16_structurals, stats->blocks));
        printf("special block flips: utf8 %9zu (%5.1f %%) - escape %9zu (%5.1f %%) - 0 structurals %9zu (%5.1f %%) - 1+ structurals %9zu (%5.1f %%) - 8+ structurals %9zu (%5.1f %%) - 16+ structurals %9zu (%5.1f %%)\n",
          stats->blocks_with_utf8_flipped, percent(stats->blocks_with_utf8_flipped, stats->blocks),
          stats->blocks_with_escapes_flipped, percent(stats->blocks_with_escapes_flipped, stats->blocks),
          stats->blocks_with_0_structurals_flipped, percent(stats->blocks_with_0_structurals_flipped, stats->blocks),
          stats->blocks_with_1_structural_flipped, percent(stats->blocks_with_1_structural_flipped, stats->blocks),
          stats->blocks_with_8_structurals_flipped, percent(stats->blocks_with_8_structurals_flipped, stats->blocks),
          stats->blocks_with_16_structurals_flipped, percent(stats->blocks_with_16_structurals_flipped, stats->blocks));
      }
      printf("\n");
      if(!stage1_only) {
        printf("All Stages (excluding allocation)\n");
        print_aggregate("|    "   , all_stages_without_allocation.best);
        // frequently, allocation is a tiny fraction of the running time so we omit it
        if(allocate_stage.best.elapsed_sec() > 0.01 * all_stages_without_allocation.best.elapsed_sec()) {
          printf("|- Allocation\n");
          print_aggregate("|    ", allocate_stage.best);
        }
      }
      printf("|- Stage 1\n");
      print_aggregate("|    ", stage1.best);
      if(!stage1_only) {
        printf("|- Stage 2\n");
        print_aggregate("|    ", stage2.best);
      }
      if (collector.has_events()) {
        double freq1 = (stage1.best.cycles() / stage1.best.elapsed_sec()) / 1000000000.0;
        double freq2 = (stage2.best.cycles() / stage2.best.elapsed_sec()) / 1000000000.0;
        double freqall = (all_stages_without_allocation.best.cycles() / all_stages_without_allocation.best.elapsed_sec()) / 1000000000.0;
        double freqmin = min(freq1, freq2);
        double freqmax = max(freq1, freq2);
        if((freqall < 0.95 * freqmin) || (freqall > 1.05 * freqmax)) {
          printf("\nWarning: The processor frequency fluctuates in an expected way!!!\n"
          "Range for stage 1 and stage 2 : [%.3f GHz, %.3f GHz], overall: %.3f GHz.\n",
          freqmin, freqmax, freqall);
        }
      }
      printf("\n%.1f documents parsed per second (best)\n", 1.0/static_cast<double>(all_stages_without_allocation.best.elapsed_sec()));
    }
  }
};

#endif
