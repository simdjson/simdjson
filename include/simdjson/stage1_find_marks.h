#pragma once

#include "common_defs.h"
#include "parsedjson.h"

WARN_UNUSED
bool find_structural_bits(const u8 *buf, size_t len, ParsedJson &pj);
