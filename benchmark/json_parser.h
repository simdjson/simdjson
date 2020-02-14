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
using jsonparse_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj, bool streaming);

stage1_functype* get_stage1_func(const architecture arch) {
  switch (arch) {
  #ifdef IS_X86_64
    case architecture::HASWELL:
      return &find_structural_bits<architecture::HASWELL>;
    case architecture::WESTMERE:
      return &find_structural_bits<architecture::WESTMERE>;
  #endif
  #ifdef IS_ARM64
    case architecture::ARM64:
      return &find_structural_bits<architecture::ARM64>;
  #endif
  default:
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    exit(EXIT_FAILURE);
  }
}

stage2_functype* get_stage2_func(const architecture arch) {
  switch (arch) {
#ifdef IS_X86_64
  case architecture::HASWELL:
    return &unified_machine<architecture::HASWELL>;
    break;
  case architecture::WESTMERE:
    return &unified_machine<architecture::WESTMERE>;
    break;
#endif
#ifdef IS_ARM64
  case architecture::ARM64:
    return &unified_machine<architecture::ARM64>;
    break;
#endif
  default:
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    exit(EXIT_FAILURE);
  }
}

jsonparse_functype* get_jsonparse_func(const architecture arch) {
  switch (arch) {
#ifdef IS_X86_64
  case architecture::HASWELL:
    return &json_parse_implementation<architecture::HASWELL>;
    break;
  case architecture::WESTMERE:
    return &json_parse_implementation<architecture::WESTMERE>;
    break;
#endif
#ifdef IS_ARM64
  case architecture::ARM64:
    return &json_parse_implementation<architecture::ARM64>;
    break;
#endif
  default:
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    exit(EXIT_FAILURE);
  }
}

struct json_parser {
  const architecture arch;
  stage1_functype *stage1_func;
  stage2_functype *stage2_func;
  jsonparse_functype *jsonparse_func;

  json_parser(const architecture _arch) : arch(_arch) {
    this->stage1_func = get_stage1_func(arch);
    this->stage2_func = get_stage2_func(arch);
    this->jsonparse_func = get_jsonparse_func(arch);
  }
  json_parser() : json_parser(find_best_supported_architecture()) {}

  int stage1(const uint8_t *buf, const size_t len, ParsedJson &pj) const {
    return this->stage1_func(buf, len, pj);
  }

  int stage2(const uint8_t *buf, const size_t len, ParsedJson &pj) const {
    return this->stage2_func(buf, len, pj);
  }

  int parse(const uint8_t *buf, const size_t len, ParsedJson &pj) const {
    // yes, you can construct jsonparse from stage 1 and stage 2,
    // but why emulate it when we have the real thing?
    return this->jsonparse_func(buf, len, pj, false);
  }
};

#endif