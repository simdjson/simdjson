#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <x86intrin.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>
#include <inttypes.h>

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
#include "simdjson/jsonparser.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/parsedjson.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_flatten.h"
#include "simdjson/stage34_unified.h"
using namespace std;

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool dump = false;
  bool jsonoutput = false;
  bool forceoneiteration = false;
  bool justdata = false;

  int c;

  while ((c = getopt (argc, argv, "1vdt")) != -1)
    switch (c)
      {
      case 't':
        justdata = true;
        break;
      case 'v':
        verbose = true;
        break;
      case 'd':
        dump = true;
        break;
      case 'j':
        jsonoutput = true;
        break;
      case '1':
        forceoneiteration = true;
        break;
      default:
        abort ();
      }
  if (optind >= argc) {
    cerr << "Usage: " << argv[0] << " <jsonfile>" << endl;
    exit(1);
  }
  const char * filename = argv[optind];
  if(optind + 1 < argc) {
    cerr << "warning: ignoring everything after " << argv[optind  + 1] << endl;
  }
  if(verbose) cout << "[verbose] loading " << filename << endl;
  std::string_view p;
  try {
    p = get_corpus(filename);
  } catch (const std::exception& e) { // caught by reference to base
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  if(verbose) cout << "[verbose] loaded " << filename << " ("<< p.size() << " bytes)" << endl;
#if defined(DEBUG)
  const u32 iterations = 1;
#else
  const u32 iterations = forceoneiteration ? 1 : ( p.size() < 1 * 1000 * 1000? 1000 : 10);
#endif
  vector<double> res;
  res.resize(iterations);

#if !defined(__linux__)
#define SQUASH_COUNTERS
  if(justdata) {
    printf("justdata (-t) flag only works under linux.\n");
  }
#endif

#ifndef SQUASH_COUNTERS
  vector<int> evts;
  evts.push_back(PERF_COUNT_HW_CPU_CYCLES);
  evts.push_back(PERF_COUNT_HW_INSTRUCTIONS);
  evts.push_back(PERF_COUNT_HW_BRANCH_MISSES);
  evts.push_back(PERF_COUNT_HW_CACHE_REFERENCES);
  evts.push_back(PERF_COUNT_HW_CACHE_MISSES);
  LinuxEvents<PERF_TYPE_HARDWARE> unified(evts);
  vector<u64> results;
  results.resize(evts.size());
  unsigned long cy0 = 0, cy1 = 0, cy2 = 0, cy3 = 0;
  unsigned long cl0 = 0, cl1 = 0, cl2 = 0, cl3 = 0;
  unsigned long mis0 = 0, mis1 = 0, mis2 = 0, mis3 = 0;
  unsigned long cref0 = 0, cref1 = 0, cref2 = 0, cref3 = 0;
  unsigned long cmis0 = 0, cmis1 = 0, cmis2 = 0, cmis3 = 0;
#endif
  bool isok = true;

  for (u32 i = 0; i < iterations; i++) {
    if(verbose) cout << "[verbose] iteration # " << i << endl;
#ifndef SQUASH_COUNTERS
    unified.start();
#endif
    ParsedJson pj;
    bool allocok = pj.allocateCapacity(p.size());
    if(!allocok) {
      std::cerr << "failed to allocate memory" << std::endl;
      return EXIT_FAILURE;
    }
#ifndef SQUASH_COUNTERS
    unified.end(results);
    cy0 += results[0];
    cl0 += results[1];
    mis0 += results[2];
    cref0 += results[3];
    cmis0 += results[4];
#endif
    if(verbose) cout << "[verbose] allocated memory for parsed JSON " << endl;

    auto start = std::chrono::steady_clock::now();
#ifndef SQUASH_COUNTERS
    unified.start();
#endif
    isok = find_structural_bits(p.data(), p.size(), pj);
#ifndef SQUASH_COUNTERS
    unified.end(results);
    cy1 += results[0];
    cl1 += results[1];
    mis1 += results[2];
    cref1 += results[3];
    cmis1 += results[4];
    if (!isok) {
      cout << "Failed out during stage 1\n";
      break;
    }
    unified.start();
#endif
    isok = isok && flatten_indexes(p.size(), pj);
#ifndef SQUASH_COUNTERS
    unified.end(results);
    cy2 += results[0];
    cl2 += results[1];
    mis2 += results[2];
    cref2 += results[3];
    cmis2 += results[4];
    if (!isok) {
      cout << "Failed out during stage 2\n";
      break;
    }
    unified.start();
#endif

    isok = isok && unified_machine(p.data(), p.size(), pj);
#ifndef SQUASH_COUNTERS
    unified.end(results);
    cy3 += results[0];
    cl3 += results[1];
    mis3 += results[2];
    cref3 += results[3];
    cmis3 += results[4];
    if (!isok) {
      cout << "Failed out during stage 34\n";
      break;
    }
#endif

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> secs = end - start;
    res[i] = secs.count();
  }
  ParsedJson pj = build_parsed_json(p); // do the parsing again to get the stats
  if( ! pj.isValid() ) {
    std::cerr << "Could not parse. " << std::endl;
    return EXIT_FAILURE;
  }
#ifndef SQUASH_COUNTERS
  if(justdata) {
    float cpb0 =  (double)cy0 / (iterations * p.size());
    float cpb1 =  (double)cy1 / (iterations * p.size());
    float cpb2 =  (double)cy2 / (iterations * p.size());
    float cpb3 =  (double)cy3 / (iterations * p.size());
    float cpbtotal = (double)total / (iterations * p.size());
    printf("\"%s\"\t%f\t%f\t%f\t%f\t%f\n", basename(filename), cpb0,cpb1,cpb2,cpb3,cpbtotal);
  } else {
  printf("number of bytes %ld number of structural chars %u ratio %.3f\n",
         p.size(), pj.n_structural_indexes,
         (double)pj.n_structural_indexes / p.size());
  unsigned long total = cy0 + cy1 + cy2 + cy3;
  printf(
      "mem alloc instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f mis. branches: %10lu (cycles/mis.branch %.2f) cache accesses: %10lu (failure %10lu)\n",
      cl0 / iterations, cy0 / iterations, 100. * cy0 / total, (double)cl0 / cy0, mis0/iterations, (double)cy0/mis0, cref1 / iterations, cmis0 / iterations);
  printf(" mem alloc runs at %.2f cycles per input byte.\n",
         (double)cy0 / (iterations * p.size()));
  printf(
      "stage 1 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f mis. branches: %10lu (cycles/mis.branch %.2f) cache accesses: %10lu (failure %10lu)\n",
      cl1 / iterations, cy1 / iterations, 100. * cy1 / total, (double)cl1 / cy1, mis1/iterations, (double)cy1/mis1, cref1 / iterations, cmis1 / iterations);
  printf(" stage 1 runs at %.2f cycles per input byte.\n",
         (double)cy1 / (iterations * p.size()));

  printf(
      "stage 2 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f mis. branches: %10lu  (cycles/mis.branch %.2f)  cache accesses: %10lu (failure %10lu)\n",
      cl2 / iterations, cy2 / iterations, 100. * cy2 / total, (double)cl2 / cy2, mis2/iterations, (double)cy2/mis2, cref2 /iterations, cmis2 / iterations);
  printf(" stage 2 runs at %.2f cycles per input byte and ",
         (double)cy2 / (iterations * p.size()));
  printf("%.2f cycles per structural character.\n",
         (double)cy2 / (iterations * pj.n_structural_indexes));

  printf(
      "stage 3 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f mis. branches: %10lu  (cycles/mis.branch %.2f)  cache accesses: %10lu (failure %10lu)\n",
      cl3 / iterations, cy3 /iterations, 100. * cy3 / total, (double)cl3 / cy3, mis3/iterations, (double)cy3/mis3, cref3 / iterations, cmis3 / iterations);
  printf(" stage 3 runs at %.2f cycles per input byte and ",
         (double)cy3 / (iterations * p.size()));
  printf("%.2f cycles per structural character.\n",
         (double)cy3 / (iterations * pj.n_structural_indexes));

  printf(" all stages: %.2f cycles per input byte.\n",
         (double)total / (iterations * p.size()));
  }
#endif
  double min_result = *min_element(res.begin(), res.end());
  if(!justdata) cout << "Min:  " << min_result << " bytes read: " << p.size()
       << " Gigabytes/second: " << (p.size()) / (min_result * 1000000000.0)
       << "\n";
  if(jsonoutput) {
    isok = isok && pj.printjson(std::cout);
  }
  if(dump) {
    isok = isok && pj.dump_raw_tape(std::cout);
  }
  free((void*)p.data());
  if (!isok) {
    printf(" Parsing failed. \n ");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
