#ifndef SIMDJSON_STAGE1_FIND_MARKS_H
#define SIMDJSON_STAGE1_FIND_MARKS_H

#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"

WARN_UNUSED
bool find_structural_bits(const uint8_t *buf, size_t len, ParsedJson &pj);

WARN_UNUSED
static inline bool find_structural_bits(const char *buf, size_t len, ParsedJson &pj) {
    return find_structural_bits((const uint8_t *)buf, len, pj);
}

#endif
