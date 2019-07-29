#include "simdjson/jsonparser.h"
#ifdef _MSC_VER
#include <sysinfoapi.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "simdjson/isadetection.h"
#include "simdjson/portability.h"
#include "simdjson/simdjson.h"

namespace simdjson {

architecture find_best_supported_implementation() {
  constexpr uint32_t haswell_flags =
      SIMDExtensions::AVX2 | SIMDExtensions::PCLMULQDQ | SIMDExtensions::BMI1 |
      SIMDExtensions::BMI2;
  constexpr uint32_t westmere_flags =
      SIMDExtensions::SSE42 | SIMDExtensions::PCLMULQDQ;

  uint32_t supports = detect_supported_architectures();
  // Order from best to worst (within architecture)
  if ((haswell_flags & supports) == haswell_flags)
    return architecture::haswell;
  if ((westmere_flags & supports) == westmere_flags)
    return architecture::westmere;
  if (SIMDExtensions::NEON)
    return architecture::arm64;

  return architecture::none;
}

// Responsible to select the best json_parse implementation
int json_parse_dispatch(const uint8_t *buf, size_t len, ParsedJson &pj,
                        bool reallocifneeded) {
  architecture best_implementation = find_best_supported_implementation();
  // Selecting the best implementation
  switch (best_implementation) {
#ifdef IS_X86_64
  case architecture::haswell:
    json_parse_ptr = &json_parse_implementation<architecture::haswell>;
    break;
  case architecture::westmere:
    json_parse_ptr = &json_parse_implementation<architecture::westmere>;
    break;
#endif
#ifdef IS_ARM64
  case architecture::arm64:
    json_parse_ptr = &json_parse_implementation<architecture::arm64>;
    break;
#endif
  default:
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    return simdjson::UNEXPECTED_ERROR;
  }

  return json_parse_ptr(buf, len, pj, reallocifneeded);
}

json_parse_functype *json_parse_ptr = &json_parse_dispatch;

WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len,
                             bool reallocifneeded) {
  ParsedJson pj;
  bool ok = pj.allocateCapacity(len);
  if (ok) {
    json_parse(buf, len, pj, reallocifneeded);
  } else {
    std::cerr << "failure during memory allocation " << std::endl;
  }
  return pj;
}
} // namespace simdjson
