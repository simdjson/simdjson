#ifndef SIMDJSON_STAGE1_FIND_MARKS_H
#define SIMDJSON_STAGE1_FIND_MARKS_H

#include "simdjson/common_defs.h"

struct ParsedJson;

WARN_UNUSED
bool find_structural_bits(const uint8_t *buf, size_t len, ParsedJson &pj);

WARN_UNUSED
bool find_structural_bits(const char *buf, size_t len, ParsedJson &pj);

#endif
