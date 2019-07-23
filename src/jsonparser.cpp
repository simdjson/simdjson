#include "simdjson/jsonparser.h"
#ifdef _MSC_VER
#include <windows.h>
#include <sysinfoapi.h>
#else
#include <unistd.h>
#endif
#include "simdjson/simdjson.h"
#include "simdjson/simddetection.h"
#include "simdjson/portability.h"

namespace simdjson {

instruction_set find_best_supported_implementation() {
  uint32_t supports = detectHostSIMDExtensions();
  // Order from best to worst (within architecture)
  if (supports & SIMDExtensions::AVX2) return instruction_set::avx2;
  if (supports & SIMDExtensions::SSE42) return instruction_set::sse4_2;
  if (supports & SIMDExtensions::NEON) return instruction_set::neon;

  return instruction_set::none;
}

// Responsible to select the best json_parse implementation
int json_parse_dispatch(const uint8_t *buf, size_t len, ParsedJson &pj, bool reallocifneeded) {
  instruction_set best_implementation = find_best_supported_implementation();
  // Selecting the best implementation
  switch (best_implementation) {
#ifdef IS_x86_64 // TODO this only needs to be IS_X86 -- CPUID is safe on 32-bit
  case instruction_set::avx2:
    json_parse_ptr = &json_parse_implementation<instruction_set::avx2>;
    break;
  case instruction_set::sse4_2:
    json_parse_ptr = &json_parse_implementation<instruction_set::sse4_2>;
    break;
#endif
#ifdef IS_ARM64
  case instruction_set::neon:
    json_parse_ptr = &json_parse_implementation<instruction_set::neon>;
    break;
#endif
  default :
    std::cerr << "No implemented simd instruction set supported" << std::endl;
    return simdjson::UNEXPECTED_ERROR;
  }

  return json_parse_ptr(buf, len, pj, reallocifneeded);
}

json_parse_functype *json_parse_ptr = &json_parse_dispatch;

WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len, bool reallocifneeded) {
  ParsedJson pj;
  bool ok = pj.allocateCapacity(len);
  if(ok) {
    json_parse(buf, len, pj, reallocifneeded);
  } else {
    std::cerr << "failure during memory allocation " << std::endl;
  }
  return pj;
}
}
