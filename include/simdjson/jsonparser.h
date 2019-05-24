#ifndef SIMDJSON_JSONPARSER_H
#define SIMDJSON_JSONPARSER_H
#include <string>
#include "simdjson/common_defs.h"
#include "simdjson/padded_string.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/parsedjson.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_build_tape.h"
#include "simdjson/simdjson.h"

// Parse a document found in buf. 
// You need to preallocate ParsedJson with a capacity of len (e.g., pj.allocateCapacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success or an error code from 
// simdjson/simdjson.h in case of failure such as  simdjson::CAPACITY, simdjson::MEMALLOC, 
// simdjson::DEPTH_ERROR and so forth; the simdjson::errorMsg function converts these error codes 
// into a string). 
//
// You can also check validity by calling pj.isValid(). The same ParsedJson can be reused for other documents.
//
// If reallocifneeded is true (default) then a temporary buffer is created when needed during processing
// (a copy of the input string is made).
// The input buf should be readable up to buf + len + SIMDJSON_PADDING if reallocifneeded is false,
// all bytes at and after buf + len  are ignored (can be garbage).
// The ParsedJson object can be reused.
WARN_UNUSED
int json_parse(const uint8_t *buf, size_t len, ParsedJson &pj, bool reallocifneeded = true);

// Parse a document found in buf.
// You need to preallocate ParsedJson with a capacity of len (e.g., pj.allocateCapacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success or an error code from 
// simdjson/simdjson.h in case of failure such as  simdjson::CAPACITY, simdjson::MEMALLOC, 
// simdjson::DEPTH_ERROR and so forth; the simdjson::errorMsg function converts these error codes 
// into a string). 
//
// You can also check validity
// by calling pj.isValid(). The same ParsedJson can be reused for other documents.
//
// If reallocifneeded is true (default) then a temporary buffer is created when needed during processing
// (a copy of the input string is made).
// The input buf should be readable up to buf + len + SIMDJSON_PADDING  if reallocifneeded is false,
// all bytes at and after buf + len  are ignored (can be garbage).
// The ParsedJson object can be reused.
WARN_UNUSED
inline int json_parse(const char * buf, size_t len, ParsedJson &pj, bool reallocifneeded = true) {
  return json_parse(reinterpret_cast<const uint8_t *>(buf), len, pj, reallocifneeded);
}

// We do not want to allow implicit conversion from C string to std::string.
int json_parse(const char * buf, ParsedJson &pj) = delete;

// Parse a document found in in string s.
// You need to preallocate ParsedJson with a capacity of len (e.g., pj.allocateCapacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success or an error code from 
// simdjson/simdjson.h in case of failure such as  simdjson::CAPACITY, simdjson::MEMALLOC, 
// simdjson::DEPTH_ERROR and so forth; the simdjson::errorMsg function converts these error codes 
// into a string). 
//
// A temporary buffer is created when needed during processing
// (a copy of the input string is made).
WARN_UNUSED
inline int json_parse(const std::string &s, ParsedJson &pj) {
  return json_parse(s.data(), s.length(), pj, true);
}

// Parse a document found in in string s.
// You need to preallocate ParsedJson with a capacity of len (e.g., pj.allocateCapacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success or an error code from 
// simdjson/simdjson.h in case of failure such as  simdjson::CAPACITY, simdjson::MEMALLOC, 
// simdjson::DEPTH_ERROR and so forth; the simdjson::errorMsg function converts these error codes 
// into a string). 
//
// You can also check validity
// by calling pj.isValid(). The same ParsedJson can be reused for other documents.
WARN_UNUSED
inline int json_parse(const padded_string &s, ParsedJson &pj) {
  return json_parse(s.data(), s.length(), pj, false);
}


// Build a ParsedJson object. You can check validity
// by calling pj.isValid(). This does the memory allocation needed for ParsedJson.
// If reallocifneeded is true (default) then a temporary buffer is created when needed during processing
// (a copy of the input string is made).
//
// the input buf should be readable up to buf + len + SIMDJSON_PADDING  if reallocifneeded is false,
// all bytes at and after buf + len  are ignored (can be garbage).
//
// This is a convenience function which calls json_parse.
WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len, bool reallocifneeded = true);

WARN_UNUSED
// Build a ParsedJson object. You can check validity
// by calling pj.isValid(). This does the memory allocation needed for ParsedJson.
// If reallocifneeded is true (default) then a temporary buffer is created when needed during processing
// (a copy of the input string is made).
// The input buf should be readable up to buf + len + SIMDJSON_PADDING if reallocifneeded is false,
// all bytes at and after buf + len  are ignored (can be garbage).
//
// This is a convenience function which calls json_parse.
inline ParsedJson build_parsed_json(const char * buf, size_t len, bool reallocifneeded = true) {
  return build_parsed_json(reinterpret_cast<const uint8_t *>(buf), len, reallocifneeded);
}


// We do not want to allow implicit conversion from C string to std::string.
ParsedJson build_parsed_json(const char *buf) = delete;


// Parse a document found in in string s.
// You need to preallocate ParsedJson with a capacity of len (e.g., pj.allocateCapacity(len)).
// Return SUCCESS (an integer = 0) in case of a success. You can also check validity
// by calling pj.isValid(). The same ParsedJson can be reused for other documents.
//
// A temporary buffer is created when needed during processing
// (a copy of the input string is made).
//
// This is a convenience function which calls json_parse.
WARN_UNUSED
inline ParsedJson build_parsed_json(const std::string &s) {
  return build_parsed_json(s.data(), s.length(), true);
}


// Parse a document found in in string s.
// You need to preallocate ParsedJson with a capacity of len (e.g., pj.allocateCapacity(len)).
// Return SUCCESS (an integer = 0) in case of a success. You can also check validity
// by calling pj.isValid(). The same ParsedJson can be reused for other documents.
//
// This is a convenience function which calls json_parse.
WARN_UNUSED
inline ParsedJson build_parsed_json(const padded_string &s) {
  return build_parsed_json(s.data(), s.length(), false);
}



#endif
