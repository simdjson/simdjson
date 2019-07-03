#include "simdjson/jsonparser.h"
#ifdef _MSC_VER
#include <windows.h>
#include <sysinfoapi.h>
#else
#include <unistd.h>
#endif
#include "simdjson/simdjson.h"

namespace simdjson {
// Responsible to select the best json_parse implementation
int json_parse_dispatch(const uint8_t *buf, size_t len, ParsedJson &pj, bool reallocifneeded) {
  // Versions for each implementation
#ifdef __AVX2__
  json_parse_functype* avx_implementation = &json_parse_implementation<instruction_set::avx2>;
#endif
#ifdef __SSE4_2__
  // json_parse_functype* sse4_2_implementation = &json_parse_implementation<instruction_set::sse4_2>; // not implemented yet
#endif
#ifdef __ARM_NEON
  json_parse_functype* neon_implementation = &json_parse_implementation<instruction_set::neon>;
#endif

  // Determining which implementation is the more suitable
  // Should be done at runtime. Does not make any sense on preprocessor.
#ifdef __AVX2__
  instruction_set best_implementation = instruction_set::avx2;
#elif defined (__SSE4_2__)
  instruction_set best_implementation = instruction_set::sse4_2;
#elif defined (__ARM_NEON)
  instruction_set best_implementation = instruction_set::neon;
#else
  instruction_set best_implementation = instruction_set::none;
#endif
  
  // Selecting the best implementation
  switch (best_implementation) {
#ifdef __AVX2__
  case instruction_set::avx2 :
    json_parse_ptr = avx_implementation;
    break;
#elif defined (__SSE4_2__)
  /*case instruction_set::sse4_2 :
    json_parse_ptr = sse4_2_implementation;
    break;*/
#elif defined (__ARM_NEON)
  case instruction_set::neon :
    json_parse_ptr = neon_implementation;
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