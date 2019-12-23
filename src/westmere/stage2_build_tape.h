#ifndef SIMDJSON_WESTMERE_STAGE2_BUILD_TAPE_H
#define SIMDJSON_WESTMERE_STAGE2_BUILD_TAPE_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "simdjson/stage2_build_tape.h"
#include "westmere/stringparsing.h"
#include "westmere/numberparsing.h"

TARGET_WESTMERE
namespace simdjson::westmere {

#include "generic/stage2_build_tape.h"
#include "generic/stage2_streaming_build_tape.h"

} // namespace simdjson::westmere
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {

template <>
WARN_UNUSED int
unified_machine<Architecture::WESTMERE>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  westmere::stage2_status status;
  int ret = westmere::unified_machine_init(buf, len, pj, status);
  while(ret == simdjson::SUCCESS_AND_HAS_MORE) {
    ret = westmere::unified_machine_continue(buf, len, pj, status, pj.n_structural_indexes);
  }
  return ret;
}

template <>
WARN_UNUSED int
unified_machine<Architecture::WESTMERE>(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
    return westmere::unified_machine(buf, len, pj, next_json);
}


} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64

#endif // SIMDJSON_WESTMERE_STAGE2_BUILD_TAPE_H
