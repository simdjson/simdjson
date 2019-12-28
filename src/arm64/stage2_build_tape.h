#ifndef SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
#define SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H

#include "simdjson/portability.h"

#ifdef IS_ARM64

#include "simdjson/stage2_build_tape.h"
#include "arm64/stringparsing.h"
#include "arm64/numberparsing.h"

namespace simdjson::arm64 {

#include "generic/stage2_build_tape.h"
#include "generic/stage2_streaming_build_tape.h"

} // namespace simdjson::arm64

namespace simdjson {

template <>
WARN_UNUSED int
unified_machine<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  arm64::stage2_status status;
  int ret = arm64::unified_machine_init(buf, len, pj, status);
  if(ret == simdjson::SUCCESS_AND_HAS_MORE) {
    ret = arm64::unified_machine_continue<arm64::check_index_end_false>(buf, len, pj, status, pj.n_structural_indexes);
  }
  return ret;
}

template <>
WARN_UNUSED int
unified_machine<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
    return arm64::unified_machine(buf, len, pj, next_json);
}

template <>
WARN_UNUSED  int
unified_machine_init<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status& s) {
    return arm64::unified_machine_init(buf, len, pj, s);
}

template <>
WARN_UNUSED  int
unified_machine_continue<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status &s, size_t last_index) {
    return arm64::unified_machine_continue<true>(buf, len, pj, s, last_index);
}

template <>
WARN_UNUSED  int
unified_machine_finish<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status &s) {
    return arm64::unified_machine_continue<false>(buf, len, pj, s, pj.n_structural_indexes);
}




} // namespace simdjson

#endif // IS_ARM64

#endif // SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
