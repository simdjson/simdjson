#ifndef SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
#define SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H

#include "simdjson/portability.h"

#ifdef IS_ARM64

#include "parsed_json_writer.h"
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
  return arm64::stage2::unified_machine(buf, len, pj);
}

template <>
WARN_UNUSED int
unified_machine<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
    return arm64::stage2::unified_machine(buf, len, pj, next_json);
}

} // namespace simdjson

#endif // IS_ARM64

#endif // SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
