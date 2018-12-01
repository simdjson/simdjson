#pragma once

#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"

void init_state_machine();

WARN_UNUSED
bool unified_machine(const u8 *buf, size_t len, ParsedJson &pj);

WARN_UNUSED
static inline bool unified_machine(const char *buf, size_t len, ParsedJson &pj) {
    return unified_machine((const u8 *)buf,len,pj);
}

