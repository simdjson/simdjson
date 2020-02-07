#include "simdjson/jsonparser.h"
#include "simdjson/isadetection.h"
#include "simdjson/portability.h"
#include "simdjson/simdjson.h"
#include <atomic>

namespace simdjson {

// The function that users are expected to call is json_parse.
// We have more than one such function because we want to support several
// instruction sets.

// function pointer type for json_parse
using json_parse_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj, bool realloc);

// Pointer that holds the json_parse implementation corresponding to the
// available SIMD instruction set
extern std::atomic<json_parse_functype *> json_parse_ptr;

int json_parse(const uint8_t *buf, size_t len, ParsedJson &pj, bool realloc) {
  return json_parse_ptr.load(std::memory_order_relaxed)(buf, len, pj, realloc);
}

int json_parse(const char *buf, size_t len, ParsedJson &pj, bool realloc) {
  return json_parse_ptr.load(std::memory_order_relaxed)(reinterpret_cast<const uint8_t *>(buf), len, pj,
                                                        realloc);
}

architecture find_best_supported_architecture() {
  constexpr uint32_t haswell_flags =
      instruction_set::AVX2 | instruction_set::PCLMULQDQ |
      instruction_set::BMI1 | instruction_set::BMI2;
  constexpr uint32_t westmere_flags =
      instruction_set::SSE42 | instruction_set::PCLMULQDQ;

  uint32_t supports = detect_supported_architectures();
  // Order from best to worst (within architecture)
  if ((haswell_flags & supports) == haswell_flags)
    return architecture::HASWELL;
  if ((westmere_flags & supports) == westmere_flags)
    return architecture::WESTMERE;
  if (supports & instruction_set::NEON)
    return architecture::ARM64;

  return architecture::UNSUPPORTED;
}

architecture parse_architecture(char *arch) {
  if (!strcmp(arch, "HASWELL")) { return architecture::HASWELL; }
  if (!strcmp(arch, "WESTMERE")) { return architecture::WESTMERE; }
  if (!strcmp(arch, "ARM64")) { return architecture::ARM64; }
  return architecture::UNSUPPORTED;
}

// Responsible to select the best json_parse implementation
int json_parse_dispatch(const uint8_t *buf, size_t len, ParsedJson &pj, bool realloc) {
  architecture best_implementation = find_best_supported_architecture();
  // Selecting the best implementation
  switch (best_implementation) {
#ifdef IS_X86_64
  case architecture::HASWELL:
    json_parse_ptr.store(&json_parse_implementation<architecture::HASWELL>, std::memory_order_relaxed);
    break;
  case architecture::WESTMERE:
    json_parse_ptr.store(&json_parse_implementation<architecture::WESTMERE>, std::memory_order_relaxed);
    break;
#endif
#ifdef IS_ARM64
  case architecture::ARM64:
    json_parse_ptr.store(&json_parse_implementation<architecture::ARM64>, std::memory_order_relaxed);
    break;
#endif
  default:
    // The processor is not supported by simdjson.
    return simdjson::UNEXPECTED_ERROR;
  }

  return json_parse_ptr.load(std::memory_order_relaxed)(buf, len, pj, realloc);
}

std::atomic<json_parse_functype *> json_parse_ptr{&json_parse_dispatch};

WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len, bool realloc) {
  ParsedJson pj;
  bool ok = pj.allocate_capacity(len);
  if (ok) {
    json_parse(buf, len, pj, realloc);
  } 
  return pj;
}
} // namespace simdjson
