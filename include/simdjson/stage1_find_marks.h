#pragma once

#include "common_defs.h"
#include "parsedjson.h"

WARN_UNUSED
bool find_structural_bits(const u8 *buf, size_t len, ParsedJson &pj);

WARN_UNUSED
static inline bool find_structural_bits(const char *buf, size_t len, ParsedJson &pj) {
    return find_structural_bits((const u8 *)buf, len, pj);
}
