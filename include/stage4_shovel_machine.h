#pragma once

#include "common_defs.h"
#include "simdjson_internal.h"

bool shovel_machine(const u8 * buf, size_t len, ParsedJson & pj);

