#ifndef __JSON_PARSER_H
#define __JSON_PARSER_H

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

using namespace simdjson;
using std::string;

using stage2_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj);
using stage1_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj);

stage1_functype* get_stage1_func(const Architecture architecture) {
  switch (architecture) {
  #ifdef IS_X86_64
    case Architecture::HASWELL:
      return &find_structural_bits<Architecture::HASWELL>;
    case Architecture::WESTMERE:
      return &find_structural_bits<Architecture::WESTMERE>;
  #endif
  #ifdef IS_ARM64
    case Architecture::ARM64:
      return &find_structural_bits<Architecture::ARM64>;
  #endif
  default:
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    exit(EXIT_FAILURE);
  }
}

stage2_functype* get_stage2_func(const Architecture architecture) {
  switch (architecture) {
#ifdef IS_X86_64
  case Architecture::HASWELL:
    return &unified_machine<Architecture::HASWELL>;
    break;
  case Architecture::WESTMERE:
    return &unified_machine<Architecture::WESTMERE>;
    break;
#endif
#ifdef IS_ARM64
  case Architecture::ARM64:
    return &unified_machine<Architecture::ARM64>;
    break;
#endif
  default:
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    exit(EXIT_FAILURE);
  }
}

struct json_parser {
  const Architecture architecture;
  const stage1_functype *stage1_func;
  const stage2_functype *stage2_func;

  json_parser(const Architecture _architecture) : architecture(_architecture) {
    this->stage1_func = get_stage1_func(architecture);
    this->stage2_func = get_stage2_func(architecture);
  }
  json_parser() : json_parser(find_best_supported_architecture()) {}

  int stage1(const uint8_t *buf, const size_t len, ParsedJson &pj) const {
    return this->stage1_func(buf, len, pj);
  }
  int stage2(const uint8_t *buf, const size_t len, ParsedJson &pj) const {
    return this->stage2_func(buf, len, pj);
  }

  int parse(const uint8_t *buf, const size_t len, ParsedJson &pj) const {
    int result = this->stage1(buf, len, pj);
    if (result == SUCCESS) {
      result = this->stage2(buf, len, pj);
    }
    return result;
  }
};

#endif