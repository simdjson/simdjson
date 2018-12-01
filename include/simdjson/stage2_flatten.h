#pragma once

#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"

WARN_UNUSED
bool flatten_indexes(size_t len, ParsedJson &pj);
