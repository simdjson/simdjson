#ifndef SIMDJSON_JSONPARSER_H
#define SIMDJSON_JSONPARSER_H

#include "simdjson/common_defs.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/parsedjson.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_build_tape.h"
#include "simdjson/simdjson.h"

// Parse a document found in buf, need to preallocate ParsedJson.
// Return 0 on success, an error code from simdjson/simdjson.h otherwise
// You can also check validit by calling pj.isValid(). The same ParsedJson can be reused for other documents.
//
// If reallocifneeded is true (default) then a temporary buffer is created when needed during processing
// (a copy of the input string is made).
// The input buf should be readable up to buf + len + SIMDJSON_PADDING if reallocifneeded is false,
// all bytes at and after buf + len  are ignored (can be garbage).
WARN_UNUSED
int json_parse(const uint8_t *buf, size_t len, ParsedJson &pj, bool reallocifneeded = true);

// Parse a document found in buf, need to preallocate ParsedJson.
// Return SUCCESS (an integer = 1) in case of a success. You can also check validity
// by calling pj.isValid(). The same ParsedJson can be reused for other documents.
//
// If reallocifneeded is true (default) then a temporary buffer is created when needed during processing
// (a copy of the input string is made).
// The input buf should be readable up to buf + len + SIMDJSON_PADDING  if reallocifneeded is false,
// all bytes at and after buf + len  are ignored (can be garbage).
WARN_UNUSED
inline int json_parse(const char * buf, size_t len, ParsedJson &pj, bool reallocifneeded = true) {
  return json_parse(reinterpret_cast<const uint8_t *>(buf), len, pj, reallocifneeded);
}

// Parse a document found in buf, need to preallocate ParsedJson.
// Return SUCCESS (an integer = 1) in case of a success. You can also check validity
// by calling pj.isValid(). The same ParsedJson can be reused for other documents.
//
// If reallocifneeded is true (default) then a temporary buffer is created when needed during processing
// (a copy of the input string is made).
// the input s should be readable up to s.data() + s.size() + SIMDJSON_PADDING  if reallocifneeded is false,
// all bytes at and after s.data()+s.size() are ignored (can be garbage).
WARN_UNUSED
inline int json_parse(const std::string_view &s, ParsedJson &pj, bool reallocifneeded = true) {
  return json_parse(s.data(), s.size(), pj, reallocifneeded);
}


// Build a ParsedJson object. You can check validity
// by calling pj.isValid(). This does the memory allocation needed for ParsedJson.
// If reallocifneeded is true (default) then a temporary buffer is created when needed during processing
// (a copy of the input string is made).
//
// the input buf should be readable up to buf + len + SIMDJSON_PADDING  if reallocifneeded is false,
// all bytes at and after buf + len  are ignored (can be garbage).
WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len, bool reallocifneeded = true);

WARN_UNUSED
// Build a ParsedJson object. You can check validity
// by calling pj.isValid(). This does the memory allocation needed for ParsedJson.
// If reallocifneeded is true (default) then a temporary buffer is created when needed during processing
// (a copy of the input string is made).
// The input buf should be readable up to buf + len + SIMDJSON_PADDING if reallocifneeded is false,
// all bytes at and after buf + len  are ignored (can be garbage).
inline ParsedJson build_parsed_json(const char * buf, size_t len, bool reallocifneeded = true) {
  return build_parsed_json(reinterpret_cast<const uint8_t *>(buf), len, reallocifneeded);
}

// convenience function
WARN_UNUSED
// Build a ParsedJson object. You can check validity
// by calling pj.isValid(). This does the memory allocation needed for ParsedJson.
// If reallocifneeded is true (default) then a temporary buffer is created when needed during processing
// (a copy of the input string is made).
// The input s should be readable up to s.data() + s.size() + SIMDJSON_PADDING  if reallocifneeded is false,
// all bytes at and after s.data()+s.size() are ignored (can be garbage).
inline ParsedJson build_parsed_json(const std::string_view &s, bool reallocifneeded = true) {
  return build_parsed_json(s.data(), s.size(), reallocifneeded);
}

#endif
