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


//#define DEBUG

#include "jsonparser/jsonioutil.h"
#include "jsonparser/simdjson_internal.h"
#include "jsonparser/stage1_find_marks.h"
#include "jsonparser/stage2_flatten.h"
#include "jsonparser/stage34_unified.h"
using namespace std;

// https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
namespace Color {
enum Code {
  FG_DEFAULT = 39,
  FG_BLACK = 30,
  FG_RED = 31,
  FG_GREEN = 32,
  FG_YELLOW = 33,
  FG_BLUE = 34,
  FG_MAGENTA = 35,
  FG_CYAN = 36,
  FG_LIGHT_GRAY = 37,
  FG_DARK_GRAY = 90,
  FG_LIGHT_RED = 91,
  FG_LIGHT_GREEN = 92,
  FG_LIGHT_YELLOW = 93,
  FG_LIGHT_BLUE = 94,
  FG_LIGHT_MAGENTA = 95,
  FG_LIGHT_CYAN = 96,
  FG_WHITE = 97,
  BG_RED = 41,
  BG_GREEN = 42,
  BG_BLUE = 44,
  BG_DEFAULT = 49
};
class Modifier {
  Code code;

public:
  Modifier(Code pCode) : code(pCode) {}
  friend std::ostream &operator<<(std::ostream &os, const Modifier &mod) {
    return os << "\033[" << mod.code << "m";
  }
};
} // namespace Color

void colorfuldisplay(ParsedJson &pj, const u8 *buf) {
  Color::Modifier greenfg(Color::FG_GREEN);
  Color::Modifier yellowfg(Color::FG_YELLOW);
  Color::Modifier deffg(Color::FG_DEFAULT);
  size_t i = 0;
  // skip initial fluff
  while ((i + 1 < pj.n_structural_indexes) &&
         (pj.structural_indexes[i] == pj.structural_indexes[i + 1])) {
    i++;
  }
  for (; i < pj.n_structural_indexes; i++) {
    u32 idx = pj.structural_indexes[i];
    u8 c = buf[idx];
    if (((c & 0xdf) == 0x5b)) { // meaning 7b or 5b, { or [
      std::cout << greenfg << buf[idx] << deffg;
    } else if (((c & 0xdf) == 0x5d)) { // meaning 7d or 5d, } or ]
      std::cout << greenfg << buf[idx] << deffg;
    } else {
      std::cout << yellowfg << buf[idx] << deffg;
    }
    if (i + 1 < pj.n_structural_indexes) {
      u32 nextidx = pj.structural_indexes[i + 1];
      for (u32 pos = idx + 1; pos < nextidx; pos++) {
        std::cout << buf[pos];
      }
    }
  }
  std::cout << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <jsonfile>" << endl;
    exit(1);
  }
  pair<u8 *, size_t> p = get_corpus(argv[1]);
  ParsedJson *pj_ptr = new ParsedJson;
  ParsedJson &pj(*pj_ptr);

  if (posix_memalign((void **)&pj.structurals, 8,
                     ROUNDUP_N(p.second, 64) / 8)) {
    cerr << "Could not allocate memory" << endl;
    exit(1);
  };

  if (p.second > 0xffffff) {
    cerr << "Currently only support JSON files < 16MB\n";
    exit(1);
  }

  pj.n_structural_indexes = 0;
  // we have potentially 1 structure per byte of input
  // as well as a dummy structure and a root structure
  // we also potentially write up to 7 iterations beyond
  // in our 'cheesy flatten', so make some worst-case
  // space for that too
  u32 max_structures = ROUNDUP_N(p.second, 64) + 2 + 7;
  pj.structural_indexes = new u32[max_structures];

#if defined(DEBUG)
  const u32 iterations = 1;
#else
  const u32 iterations = 1000;
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
    isok = flatten_indexes(p.second, pj);
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

    isok = unified_machine(p.first, p.second, pj);
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
  printf("number of bytes %ld number of structural chars %d ratio %.3f\n",
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

  free(pj.structurals);
  free(p.first);
  delete[] pj.structural_indexes;
  delete pj_ptr;
  if (!isok) {
    printf(" Parsing failed. \n ");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
