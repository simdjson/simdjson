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
  return arm64::unified_machine(buf, len, pj);
}

template <>
WARN_UNUSED int
unified_machine<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
    return arm64::unified_machine(buf, len, pj, next_json);
}

/**
* The unified_machine_init, unified_machine_continue and unified_machine_finish
* functions are meant to support interruptible and interleaved processing.
*/

template <>
WARN_UNUSED  int
unified_machine_init<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj) {
    return arm64::unified_machine_init(buf, len, pj);
}

template <>
WARN_UNUSED  int
unified_machine_continue<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj, size_t last_index) {
    return arm64::unified_machine_continue(buf, len, pj,last_index);
}

template <>
WARN_UNUSED  int
unified_machine_finish<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj) {
    return arm64::unified_machine_continue(buf, len, pj, pj.n_structural_indexes);
}




} // namespace simdjson

#endif // IS_ARM64

#endif // SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
