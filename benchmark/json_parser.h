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

namespace simdjson {
    // to be moved to top level include
namespace architecture_services {

using stage2_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj);
using stage1_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj);

stage1_functype *get_stage1_func(const Architecture architecture) {
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

stage2_functype *get_stage2_func(const Architecture architecture) {
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

inline auto stage1_func = [](Architecture const &arch_) {
  // once
  static auto holder_ = [func_ = get_stage1_func(arch_)](
                            const uint8_t *buf, const size_t len,
                            ParsedJson &pj) { return func_(buf, len, pj); };
  return holder_;
};

inline auto stage2_func = [](Architecture const &arch_) {
  // once
  static auto holder_ = [func_ = get_stage2_func(arch_)](
                            const uint8_t *buf, const size_t len,
                            ParsedJson &pj) { return func_(buf, len, pj); };
  return holder_;
};
} // namespace architecture_services
} // namespace simdjson

namespace {
    // NOTE: there are no values in this anon. namespace
    // thus no static linkage
    using namespace simdjson ;
struct json_parser {
  const Architecture architecture;

  json_parser(const Architecture arch_) : architecture(arch_){}

  json_parser() : json_parser(find_best_supported_architecture()) {}

  int stage1(const uint8_t *buf, const size_t len, ParsedJson &pj) const {
    static auto proxy_ = simdjson::architecture_services::stage1_func(architecture);
    return proxy_(buf, len, pj);
  }
  int stage2(const uint8_t *buf, const size_t len, ParsedJson &pj) const {
    static auto proxy_ =
        simdjson::architecture_services::stage2_func(architecture);
    return proxy_(buf, len, pj);
  }

  int parse(const uint8_t *buf, const size_t len, ParsedJson &pj) const {
    int result = this->stage1(buf, len, pj);
    if (result == SUCCESS) {
      result = this->stage2(buf, len, pj);
    }
    return result;
  }
}; // json_parser
} // namespace

#endif