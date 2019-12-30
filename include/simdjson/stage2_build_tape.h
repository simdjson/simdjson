#ifndef SIMDJSON_STAGE2_BUILD_TAPE_H
#define SIMDJSON_STAGE2_BUILD_TAPE_H

#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"
#include "simdjson/simdjson.h"

namespace simdjson {

void init_state_machine();

template <Architecture T = Architecture::NATIVE>
WARN_UNUSED int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj);

template <Architecture T = Architecture::NATIVE>
int unified_machine(const char *buf, size_t len, ParsedJson &pj) {
  return unified_machine<T>(reinterpret_cast<const uint8_t *>(buf), len, pj);
}


// more flexible version


/**
* The unified_machine_init, unified_machine_continue and unified_machine_finish
* functions are meant to support interruptible and interleaved processing.
*/

template <Architecture T = Architecture::NATIVE>
WARN_UNUSED  int
unified_machine_init(const uint8_t *buf, size_t len, ParsedJson &pj);

template <Architecture T = Architecture::NATIVE>
WARN_UNUSED  int
unified_machine_continue(const uint8_t *buf, size_t len, ParsedJson &pj, size_t last_index);

template <Architecture T = Architecture::NATIVE>
WARN_UNUSED  int
unified_machine_finish(const uint8_t *buf, size_t len, ParsedJson &pj);



// Streaming
template <Architecture T = Architecture::NATIVE>
WARN_UNUSED int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json);

template <Architecture T = Architecture::NATIVE>
int unified_machine(const char *buf, size_t len, ParsedJson &pj, size_t &next_json) {
    return unified_machine<T>(reinterpret_cast<const uint8_t *>(buf), len, pj, next_json);
}


} // namespace simdjson

#endif
