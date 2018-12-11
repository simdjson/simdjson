#pragma once

#include "simdjson/common_defs.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/parsedjson.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_flatten.h"
#include "simdjson/stage34_unified.h"




// Parse a document found in buf, need to preallocate ParsedJson.
// Return false in case of a failure. You can also check validity 
// by calling pj.isValid(). The same ParsedJson can be reused.
// the input buf should be readable up to buf + len + SIMDJSON_PADDING 
// all bytes at and after buf + len  are ignored (can be garbage)
WARN_UNUSED
bool json_parse(const u8 *buf, size_t len, ParsedJson &pj);

// the input buf should be readable up to buf + len + SIMDJSON_PADDING 
// all bytes at and after buf + len  are ignored (can be garbage)
WARN_UNUSED
static inline bool json_parse(const char * buf, size_t len, ParsedJson &pj) {
  return json_parse((const u8 *) buf, len, pj);
}

// convenience function
// the input s should be readable up to s.data() + s.size() + SIMDJSON_PADDING 
// all bytes at and after s.data()+s.size() are ignored (can be garbage)
WARN_UNUSED
static inline bool json_parse(const std::string_view &s, ParsedJson &pj) {
  return json_parse(s.data(), s.size(), pj);
}


// Build a ParsedJson object. You can check validity 
// by calling pj.isValid(). This does memory allocation.
// the input buf should be readable up to buf + len + SIMDJSON_PADDING 
// all bytes at and after buf + len  are ignored (can be garbage)
WARN_UNUSED
ParsedJson build_parsed_json(const u8 *buf, size_t len);

WARN_UNUSED
// the input buf should be readable up to buf + len + SIMDJSON_PADDING 
// all bytes at and after buf + len  are ignored (can be garbage)
static inline ParsedJson build_parsed_json(const char * buf, size_t len) {
  return build_parsed_json((const u8 *) buf, len);
}

// convenience function
WARN_UNUSED
// the input s should be readable up to s.data() + s.size() + SIMDJSON_PADDING 
// all bytes at and after s.data()+s.size() are ignored (can be garbage)
static inline ParsedJson build_parsed_json(const std::string_view &s) {
  return build_parsed_json(s.data(), s.size());
}