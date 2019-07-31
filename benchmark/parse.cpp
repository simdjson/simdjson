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
namespace simdjson {
Architecture _find_best_supported_implementation() {
  constexpr uint32_t haswell_flags =
      instruction_set::AVX2 | instruction_set::PCLMULQDQ |
      instruction_set::BMI1 | instruction_set::BMI2;
  constexpr uint32_t westmere_flags =
      instruction_set::SSE42 | instruction_set::PCLMULQDQ;
  uint32_t supports = detect_supported_architectures();
  // Order from best to worst (within architecture)
  if ((haswell_flags & supports) == haswell_flags) {
    return Architecture::HASWELL;
  }
  if ((westmere_flags & supports) == westmere_flags) {
    return Architecture::WESTMERE;
  }
  if (instruction_set::NEON)
    return Architecture::ARM64;

  return Architecture::NONE;
}

using unified_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj);
using stage1_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj);

extern unified_functype *unified_ptr;

extern stage1_functype *stage1_ptr;

int unified_machine_dispatch(const uint8_t *buf, size_t len, ParsedJson &pj) {
  Architecture best_implementation = _find_best_supported_implementation();
  // Selecting the best implementation
  switch (best_implementation) {
#ifdef IS_X86_64
  case Architecture::HASWELL:
    unified_ptr = &unified_machine<Architecture::HASWELL>;
    break;
  case Architecture::WESTMERE:
    unified_ptr = &unified_machine<Architecture::WESTMERE>;
    break;
#endif
#ifdef IS_ARM64
  case Architecture::ARM64:
    unified_ptr = &unified_machine<Architecture::ARM64>;
    break;
#endif
  default:
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    return simdjson::UNEXPECTED_ERROR;
  }

  return unified_ptr(buf, len, pj);
}

// Responsible to select the best json_parse implementation
int find_structural_bits_dispatch(const uint8_t *buf, size_t len,
                                  ParsedJson &pj) {
  Architecture best_implementation = _find_best_supported_implementation();
  // Selecting the best implementation
  switch (best_implementation) {
#ifdef IS_X86_64
  case Architecture::HASWELL:
    stage1_ptr = &find_structural_bits<Architecture::HASWELL>;
    break;
  case Architecture::WESTMERE:
    stage1_ptr = &find_structural_bits<Architecture::WESTMERE>;
    break;
#endif
#ifdef IS_ARM64
  case Architecture::ARM64:
    stage1_ptr = &find_structural_bits<Architecture::ARM64>;
    break;
#endif
  default:
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    return simdjson::UNEXPECTED_ERROR;
  }

  return stage1_ptr(buf, len, pj);
}

stage1_functype *stage1_ptr = &find_structural_bits_dispatch;
unified_functype *unified_ptr = &unified_machine_dispatch;
} // namespace simdjson

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool dump = false;
  bool json_output = false;
  bool force_one_iteration = false;
  bool just_data = false;
#ifndef _MSC_VER
  int c;

  while ((c = getopt(argc, argv, "1vdt")) != -1) {
    switch (c) {
    case 't':
      just_data = true;
      break;
    case 'v':
      verbose = true;
      break;
    case 'd':
      dump = true;
      break;
    case 'j':
      json_output = true;
      break;
    case '1':
      force_one_iteration = true;
      break;
    default:
      abort();
    }
  }
#else
  int optind = 1;
#endif
  if (optind >= argc) {
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    exit(1);
  }
  const char *filename = argv[optind];
  if (optind + 1 < argc) {
    std::cerr << "warning: ignoring everything after " << argv[optind + 1]
              << std::endl;
  }
  if (verbose) {
    std::cout << "[verbose] loading " << filename << std::endl;
  }
  simdjson::padded_string p;
  try {
    simdjson::get_corpus(filename).swap(p);
  } catch (const std::exception &) { // caught by reference to base
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  if (verbose) {
    std::cout << "[verbose] loaded " << filename << " (" << p.size()
              << " bytes)" << std::endl;
  }
#if defined(DEBUG)
  const uint32_t iterations = 1;
#else
  const uint32_t iterations =
      force_one_iteration ? 1 : (p.size() < 1 * 1000 * 1000 ? 1000 : 10);
#endif
  std::vector<double> res;
  res.resize(iterations);
  if (!just_data)
    printf("number of iterations %u \n", iterations);
#if !defined(__linux__)
#define SQUASH_COUNTERS
  if (just_data) {
    printf("just_data (-t) flag only works under linux.\n");
  }
#endif
  { // practice run
    simdjson::ParsedJson pj;
    bool allocok = pj.allocate_capacity(p.size());
    if (allocok) {
      simdjson::stage1_ptr((const uint8_t *)p.data(), p.size(), pj);
      simdjson::unified_ptr(
          (const uint8_t
               *)(const uint8_t
                      *)(const uint8_t
                             *)(const uint8_t
                                    *)(const uint8_t
                                           *)(const uint8_t
                                                  *)(const uint8_t
                                                         *)(const uint8_t *)
              p.data(),
          p.size(), pj);
    }
  }
#ifndef SQUASH_COUNTERS
  std::vector<int> evts;
  evts.push_back(PERF_COUNT_HW_CPU_CYCLES);
  evts.push_back(PERF_COUNT_HW_INSTRUCTIONS);
  evts.push_back(PERF_COUNT_HW_BRANCH_MISSES);
  evts.push_back(PERF_COUNT_HW_CACHE_REFERENCES);
  evts.push_back(PERF_COUNT_HW_CACHE_MISSES);
  LinuxEvents<PERF_TYPE_HARDWARE> unified(evts);
  std::vector<unsigned long long> results;
  results.resize(evts.size());
  unsigned long cy0 = 0, cy1 = 0, cy2 = 0;
  unsigned long cl0 = 0, cl1 = 0, cl2 = 0;
  unsigned long mis0 = 0, mis1 = 0, mis2 = 0;
  unsigned long cref0 = 0, cref1 = 0, cref2 = 0;
  unsigned long cmis0 = 0, cmis1 = 0, cmis2 = 0;
#endif
  bool isok = true;
#ifndef SQUASH_COUNTERS
  for (uint32_t i = 0; i < iterations; i++) {
    if (verbose) {
      std::cout << "[verbose] iteration # " << i << std::endl;
    }
    unified.start();
    simdjson::ParsedJson pj;
    bool allocok = pj.allocate_capacity(p.size());
    if (!allocok) {
      std::cerr << "failed to allocate memory" << std::endl;
      return EXIT_FAILURE;
    }
    unified.end(results);
    cy0 += results[0];
    cl0 += results[1];
    mis0 += results[2];
    cref0 += results[3];
    cmis0 += results[4];
    if (verbose) {
      std::cout << "[verbose] allocated memory for parsed JSON " << std::endl;
    }
    unified.start();
    isok = (simdjson::stage1_ptr((const uint8_t *)p.data(), p.size(), pj) ==
            simdjson::SUCCESS);
    unified.end(results);
    cy1 += results[0];
    cl1 += results[1];
    mis1 += results[2];
    cref1 += results[3];
    cmis1 += results[4];
    if (!isok) {
      std::cout << "Failed during stage 1" << std::endl;
      break;
    }
    unified.start();
    isok = isok &&
           (simdjson::SUCCESS ==
            simdjson::unified_ptr((const uint8_t *)p.data(), p.size(), pj));
    unified.end(results);
    cy2 += results[0];
    cl2 += results[1];
    mis2 += results[2];
    cref2 += results[3];
    cmis2 += results[4];
    if (!isok) {
      std::cout << "Failed during stage 2" << std::endl;
      break;
    }
  }
#endif
  // we do it again, this time just measuring the elapsed time
  for (uint32_t i = 0; i < iterations; i++) {
    if (verbose) {
      std::cout << "[verbose] iteration # " << i << std::endl;
    }
    simdjson::ParsedJson pj;
    bool allocok = pj.allocate_capacity(p.size());
    if (!allocok) {
      std::cerr << "failed to allocate memory" << std::endl;
      return EXIT_FAILURE;
    }
    if (verbose) {
      std::cout << "[verbose] allocated memory for parsed JSON " << std::endl;
    }

    auto start = std::chrono::steady_clock::now();
    isok = (simdjson::stage1_ptr((const uint8_t *)p.data(), p.size(), pj) ==
            simdjson::SUCCESS);
    isok = isok &&
           (simdjson::SUCCESS ==
            simdjson::unified_ptr((const uint8_t *)p.data(), p.size(), pj));
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> secs = end - start;
    res[i] = secs.count();
    if (!isok) {
      std::cerr << pj.get_error_message() << std::endl;
      std::cerr << "Could not parse. " << std::endl;
      return EXIT_FAILURE;
    }
  }
  simdjson::ParsedJson pj =
      build_parsed_json(p); // do the parsing again to get the stats
  if (!pj.is_valid()) {
    std::cerr << pj.get_error_message() << std::endl;
    std::cerr << "Could not parse. " << std::endl;
    return EXIT_FAILURE;
  }
  double min_result = *min_element(res.begin(), res.end());
  double speedinGBs = (p.size()) / (min_result * 1000000000.0);
#ifndef SQUASH_COUNTERS
  unsigned long total = cy0 + cy1 + cy2;
  if (just_data) {
    float cpb0 = (double)cy0 / (iterations * p.size());
    float cpb1 = (double)cy1 / (iterations * p.size());
    float cpb2 = (double)cy2 / (iterations * p.size());
    float cpbtotal = (double)total / (iterations * p.size());
    char *newfile = (char *)malloc(strlen(filename) + 1);
    if (newfile == NULL) {
      return EXIT_FAILURE;
    }
    ::strcpy(newfile, filename);
    char *snewfile = ::basename(newfile);
    size_t nl = strlen(snewfile);
    for (size_t j = nl - 1; j > 0; j--) {
      if (snewfile[j] == '.') {
        snewfile[j] = '\0';
        break;
      }
    }
    printf("\"%s\"\t%f\t%f\t%f\t%f\t%f\n", snewfile, cpb0, cpb1, cpb2, cpbtotal,
           speedinGBs);
    free(newfile);
  } else {
    printf("number of bytes %ld number of structural chars %u ratio %.3f\n",
           p.size(), pj.n_structural_indexes,
           (double)pj.n_structural_indexes / p.size());
    printf("mem alloc instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: "
           "%.2f mis. branches: %10lu (cycles/mis.branch %.2f) cache accesses: "
           "%10lu (failure %10lu)\n",
           cl0 / iterations, cy0 / iterations, 100. * cy0 / total,
           (double)cl0 / cy0, mis0 / iterations, (double)cy0 / mis0,
           cref1 / iterations, cmis0 / iterations);
    printf(" mem alloc runs at %.2f cycles per input byte.\n",
           (double)cy0 / (iterations * p.size()));
    printf("stage 1 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: "
           "%.2f mis. branches: %10lu (cycles/mis.branch %.2f) cache accesses: "
           "%10lu (failure %10lu)\n",
           cl1 / iterations, cy1 / iterations, 100. * cy1 / total,
           (double)cl1 / cy1, mis1 / iterations, (double)cy1 / mis1,
           cref1 / iterations, cmis1 / iterations);
    printf(" stage 1 runs at %.2f cycles per input byte.\n",
           (double)cy1 / (iterations * p.size()));

    printf("stage 2 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: "
           "%.2f mis. branches: %10lu  (cycles/mis.branch %.2f)  cache "
           "accesses: %10lu (failure %10lu)\n",
           cl2 / iterations, cy2 / iterations, 100. * cy2 / total,
           (double)cl2 / cy2, mis2 / iterations, (double)cy2 / mis2,
           cref2 / iterations, cmis2 / iterations);
    printf(" stage 2 runs at %.2f cycles per input byte and ",
           (double)cy2 / (iterations * p.size()));
    printf("%.2f cycles per structural character.\n",
           (double)cy2 / (iterations * pj.n_structural_indexes));

    printf(" all stages: %.2f cycles per input byte.\n",
           (double)total / (iterations * p.size()));
    printf("Estimated average frequency: %.3f GHz.\n",
           (double)total / (iterations * min_result * 1000000000.0));
  }
#endif
  if (!just_data) {
    std::cout << "Min:  " << min_result << " bytes read: " << p.size()
              << " Gigabytes/second: " << speedinGBs << std::endl;
  }
  if (json_output) {
    isok = isok && pj.print_json(std::cout);
  }
  if (dump) {
    isok = isok && pj.dump_raw_tape(std::cout);
  }
  if (!isok) {
    fprintf(stderr, " Parsing failed. \n ");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
