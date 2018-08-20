#pragma once

#include "common_defs.h"
#include "simdjson_internal.h"

bool find_structural_bits(const u8 *buf, size_t len, ParsedJson &pj);
