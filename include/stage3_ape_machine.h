#pragma once

#include "common_defs.h"
#include "simdjson_internal.h"

void init_state_machine();
bool ape_machine(const u8 * buf, size_t len, ParsedJson & pj);
