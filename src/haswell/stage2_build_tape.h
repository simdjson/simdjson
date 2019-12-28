#ifndef SIMDJSON_HASWELL_STAGE2_BUILD_TAPE_H
#define SIMDJSON_HASWELL_STAGE2_BUILD_TAPE_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "simdjson/stage2_build_tape.h"
#include "haswell/stringparsing.h"
#include "haswell/numberparsing.h"

TARGET_HASWELL
namespace simdjson::haswell {

#include "generic/stage2_build_tape.h"
#include "generic/stage2_streaming_build_tape.h"

} // namespace simdjson::haswell
UNTARGET_REGION

TARGET_HASWELL
namespace simdjson {

template <>
WARN_UNUSED int
unified_machine<Architecture::HASWELL>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  stage2_status status;
  int ret = haswell::unified_machine_init(buf, len, pj, status);
  if(ret == simdjson::SUCCESS_AND_HAS_MORE) {
    ret = haswell::unified_machine_continue<haswell::check_index_end_false>(buf, len, pj, status, pj.n_structural_indexes);
  }
  return ret;
}

template <>
WARN_UNUSED int
unified_machine<Architecture::HASWELL>(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
    return haswell::unified_machine(buf, len, pj, next_json);
}



template <>
WARN_UNUSED  int
unified_machine_init<Architecture::HASWELL>(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status& s) {
    return haswell::unified_machine_init(buf, len, pj, s);
}

template <>
WARN_UNUSED  int
unified_machine_continue<Architecture::HASWELL>(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status &s, size_t last_index) {
    return haswell::unified_machine_continue<true>(buf, len, pj, s, last_index);
}

template <>
WARN_UNUSED  int
unified_machine_finish<Architecture::HASWELL>(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status &s) {
    return haswell::unified_machine_continue<false>(buf, len, pj, s, pj.n_structural_indexes);
}




} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64

#endif // SIMDJSON_HASWELL_STAGE2_BUILD_TAPE_H
