#pragma once

#include "simdjson/common_defs.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/parsedjson.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_flatten.h"
#include "simdjson/stage34_unified.h"

// Parse a document found in buf, need to preallocate ParsedJson.
// Return false in case of a failure.
// The string should be NULL terminated.
bool json_parse(const u8 *buf, size_t len, ParsedJson &pj);

static inline bool json_parse(const char * buf, size_t len, ParsedJson &pj) {
  return json_parse((const u8 *) buf, len, pj);
}

// convenience function
static inline bool json_parse(const std::string_view &s, ParsedJson &pj) {
  return json_parse(s.data(), s.size(), pj);
}
