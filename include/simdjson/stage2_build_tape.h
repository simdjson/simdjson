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

// This structure is ugly and at least partly unnecessary, it should go away?
// We need to track the current depth, but not much else?
typedef struct {
  uint32_t current_depth;
  size_t current_index;
} stage2_status;

template <Architecture T = Architecture::NATIVE>
WARN_UNUSED  int
unified_machine_init(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status& s);

template <Architecture T = Architecture::NATIVE>
WARN_UNUSED  int
unified_machine_continue(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status &s, size_t last_index);

template <Architecture T = Architecture::NATIVE>
WARN_UNUSED  int
unified_machine_finish(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status &s);



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
