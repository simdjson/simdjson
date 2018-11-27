#include "jsonparser/common_defs.h"
#include "linux-perf-events.h"
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <x86intrin.h>
#include <ctype.h>

//#define DEBUG
#include "jsonparser/jsonparser.h"
#include "jsonparser/jsonioutil.h"
#include "jsonparser/simdjson_internal.h"
#include "jsonparser/stage1_find_marks.h"
#include "jsonparser/stage2_flatten.h"
#include "jsonparser/stage34_unified.h"
using namespace std;

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool dump = false;
  bool forceoneiteration = false;

  int c;

  while ((c = getopt (argc, argv, "1vd")) != -1)
    switch (c)
      {
      case 'v':
        verbose = true;
        break;
      case 'd':
        dump = true;
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
  pair<u8 *, size_t> p;
  try {
    p = get_corpus(filename);
  } catch (const std::exception& e) { // caught by reference to base
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  if(verbose) cout << "[verbose] loaded " << filename << " ("<< p.second << " bytes)" << endl;
  ParsedJson *pj_ptr = allocate_ParsedJson(p.second, 1024);
  ParsedJson &pj(*pj_ptr);
  if(verbose) cout << "[verbose] allocated memory for parsed JSON " << endl;

#if defined(DEBUG)
  const u32 iterations = 1;
#else
  const u32 iterations = forceoneiteration ? 1 : ( p.second < 1 * 1000 * 1000? 1000 : 10);
#endif
  vector<double> res;
  res.resize(iterations);

#if !defined(__linux__)
#define SQUASH_COUNTERS
#endif

#ifndef SQUASH_COUNTERS
  vector<int> evts;
  evts.push_back(PERF_COUNT_HW_CPU_CYCLES);
  evts.push_back(PERF_COUNT_HW_INSTRUCTIONS);
  evts.push_back(PERF_COUNT_HW_BRANCH_MISSES);
  LinuxEvents<PERF_TYPE_HARDWARE> unified(evts);
  vector<u64> results;
  results.resize(evts.size());
  unsigned long cy1 = 0, cy2 = 0, cy3 = 0;
  unsigned long cl1 = 0, cl2 = 0, cl3 = 0;
  unsigned long mis1 = 0, mis2 = 0, mis3 = 0;
#endif
  bool isok = true;

  for (u32 i = 0; i < iterations; i++) {
    if(verbose) cout << "[verbose] iteration # " << i << endl;
    auto start = std::chrono::steady_clock::now();
#ifndef SQUASH_COUNTERS
    unified.start();
#endif
    isok = find_structural_bits(p.first, p.second, pj);
#ifndef SQUASH_COUNTERS
    unified.end(results);
    cy1 += results[0];
    cl1 += results[1];
    mis1 += results[2];
    if (!isok) {
      cout << "Failed out during stage 1\n";
      break;
    }
    unified.start();
#endif
    isok = isok && flatten_indexes(p.second, pj);
#ifndef SQUASH_COUNTERS
    unified.end(results);
    cy2 += results[0];
    cl2 += results[1];
    mis2 += results[2];
    if (!isok) {
      cout << "Failed out during stage 2\n";
      break;
    }
    unified.start();
#endif

    isok = isok && unified_machine(p.first, p.second, pj);
#ifndef SQUASH_COUNTERS
    unified.end(results);
    cy3 += results[0];
    cl3 += results[1];
    mis3 += results[2];
    if (!isok) {
      cout << "Failed out during stage 34\n";
      break;
    }
#endif

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> secs = end - start;
    res[i] = secs.count();
  }

#ifndef SQUASH_COUNTERS
  printf("number of bytes %ld number of structural chars %u ratio %.3f\n",
         p.second, pj.n_structural_indexes,
         (double)pj.n_structural_indexes / p.second);
  unsigned long total = cy1 + cy2 + cy3;

  printf(
      "stage 1 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f mis. branches: %10lu (cycles/mis.branch %.2f) \n",
      cl1 / iterations, cy1 / iterations, 100. * cy1 / total, (double)cl1 / cy1, mis1/iterations, (double)cy1/mis1);
  printf(" stage 1 runs at %.2f cycles per input byte.\n",
         (double)cy1 / (iterations * p.second));

  printf(
      "stage 2 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f mis. branches: %10lu  (cycles/mis.branch %.2f) \n",
      cl2 / iterations, cy2 / iterations, 100. * cy2 / total, (double)cl2 / cy2, mis2/iterations, (double)cy2/mis2);
  printf(" stage 2 runs at %.2f cycles per input byte and ",
         (double)cy2 / (iterations * p.second));
  printf("%.2f cycles per structural character.\n",
         (double)cy2 / (iterations * pj.n_structural_indexes));

  printf(
      "stage 3 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f mis. branches: %10lu  (cycles/mis.branch %.2f)\n",
      cl3 / iterations, cy3 /iterations, 100. * cy3 / total, (double)cl3 / cy3, mis3/iterations, (double)cy3/mis3);
  printf(" stage 3 runs at %.2f cycles per input byte and ",
         (double)cy3 / (iterations * p.second));
  printf("%.2f cycles per structural character.\n",
         (double)cy3 / (iterations * pj.n_structural_indexes));

  printf(" all stages: %.2f cycles per input byte.\n",
         (double)total / (iterations * p.second));
#endif
  //    colorfuldisplay(pj, p.first);
  double min_result = *min_element(res.begin(), res.end());
  cout << "Min:  " << min_result << " bytes read: " << p.second
       << " Gigabytes/second: " << (p.second) / (min_result * 1000000000.0)
       << "\n";
  if(dump) pj_ptr->dump_tapes();
  free(p.first);
  deallocate_ParsedJson(pj_ptr);
  if (!isok) {
    printf(" Parsing failed. \n ");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
