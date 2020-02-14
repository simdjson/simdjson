#ifndef SIMDJSON_JSONPARSER_H
#define SIMDJSON_JSONPARSER_H
#include "simdjson/common_defs.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/padded_string.h"
#include "simdjson/parsedjson.h"
#include "simdjson/simdjson.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_build_tape.h"
#include <string>

namespace simdjson {
// json_parse_implementation is the generic function, it is specialized for
// various architectures, e.g., as
// json_parse_implementation<architecture::HASWELL> or
// json_parse_implementation<architecture::ARM64>
template <architecture T>
int json_parse_implementation(const uint8_t *buf, size_t len, document::parser &parser,
                              bool realloc_if_needed = true) {
  int result = parser.init_parse(len);
  if (result != SUCCESS) { return result; }
  bool reallocated = false;
  if (realloc_if_needed) {
      const uint8_t *tmp_buf = buf;
      buf = (uint8_t *)allocate_padded_buffer(len);
      if (buf == NULL)
        return simdjson::MEMALLOC;
      memcpy((void *)buf, tmp_buf, len);
      reallocated = true;
  }
  int stage1_err = simdjson::find_structural_bits<T>(buf, len, parser);
  if (stage1_err != simdjson::SUCCESS) {
    if (reallocated) { // must free before we exit
      aligned_free((void *)buf);
    }
    return stage1_err;
  }
  int res = unified_machine<T>(buf, len, parser);
  if (reallocated) {
    aligned_free((void *)buf);
  }
  return res;
}

// Parse a document found in buf.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// You need to preallocate document::parser with a capacity of len (e.g.,
// parser.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// You can also check validity by calling parser.is_valid(). The same document::parser can
// be reused for other documents.
//
// If realloc_if_needed is true (default) then a temporary buffer is created
// when needed during processing (a copy of the input string is made). The input
// buf should be readable up to buf + len + SIMDJSON_PADDING if
// realloc_if_needed is false, all bytes at and after buf + len  are ignored
// (can be garbage). The document::parser object can be reused.

int json_parse(const uint8_t *buf, size_t len, document::parser &parser,
               bool realloc_if_needed = true);

// Parse a document found in buf.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// You need to preallocate document::parser with a capacity of len (e.g.,
// parser.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// You can also check validity
// by calling parser.is_valid(). The same document::parser can be reused for other
// documents.
//
// If realloc_if_needed is true (default) then a temporary buffer is created
// when needed during processing (a copy of the input string is made). The input
// buf should be readable up to buf + len + SIMDJSON_PADDING  if
// realloc_if_needed is false, all bytes at and after buf + len  are ignored
// (can be garbage). The document::parser object can be reused.
int json_parse(const char *buf, size_t len, document::parser &parser,
               bool realloc_if_needed = true);

// We do not want to allow implicit conversion from C string to std::string.
int json_parse(const char *buf, document::parser &parser) = delete;

// Parse a document found in in string s.
// You need to preallocate document::parser with a capacity of len (e.g.,
// parser.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// A temporary buffer is created when needed during processing
// (a copy of the input string is made).
inline int json_parse(const std::string &s, document::parser &parser) {
  return json_parse(s.data(), s.length(), parser, true);
}

// Parse a document found in in string s.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// You need to preallocate document::parser with a capacity of len (e.g.,
// parser.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// You can also check validity
// by calling parser.is_valid(). The same document::parser can be reused for other
// documents.
inline int json_parse(const padded_string &s, document::parser &parser) {
  return json_parse(s.data(), s.length(), parser, false);
}

// Build a document::parser object. You can check validity
// by calling parser.is_valid(). This does the memory allocation needed for
// document::parser. If realloc_if_needed is true (default) then a temporary buffer is
// created when needed during processing (a copy of the input string is made).
//
// The input buf should be readable up to buf + len + SIMDJSON_PADDING  if
// realloc_if_needed is false, all bytes at and after buf + len  are ignored
// (can be garbage).
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// This is a convenience function which calls json_parse.
WARN_UNUSED
document::parser build_parsed_json(const uint8_t *buf, size_t len,
                             bool realloc_if_needed = true);

WARN_UNUSED
// Build a document::parser object. You can check validity
// by calling parser.is_valid(). This does the memory allocation needed for
// document::parser. If realloc_if_needed is true (default) then a temporary buffer is
// created when needed during processing (a copy of the input string is made).
//
// The input buf should be readable up to buf + len + SIMDJSON_PADDING if
// realloc_if_needed is false, all bytes at and after buf + len  are ignored
// (can be garbage).
//
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// This is a convenience function which calls json_parse.
inline document::parser build_parsed_json(const char *buf, size_t len,
                                    bool realloc_if_needed = true) {
  return build_parsed_json(reinterpret_cast<const uint8_t *>(buf), len,
                           realloc_if_needed);
}

// We do not want to allow implicit conversion from C string to std::string.
document::parser build_parsed_json(const char *buf) = delete;

// Parse a document found in in string s.
// You need to preallocate document::parser with a capacity of len (e.g.,
// parser.allocate_capacity(len)). Return SUCCESS (an integer = 0) in case of a
// success. You can also check validity by calling parser.is_valid(). The same
// document::parser can be reused for other documents.
//
// A temporary buffer is created when needed during processing
// (a copy of the input string is made).
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// This is a convenience function which calls json_parse.
WARN_UNUSED
inline document::parser build_parsed_json(const std::string &s) {
  return build_parsed_json(s.data(), s.length(), true);
}

// Parse a document found in in string s.
// You need to preallocate document::parser with a capacity of len (e.g.,
// parser.allocate_capacity(len)). Return SUCCESS (an integer = 0) in case of a
// success. You can also check validity by calling parser.is_valid(). The same
// document::parser can be reused for other documents.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// This is a convenience function which calls json_parse.
WARN_UNUSED
inline document::parser build_parsed_json(const padded_string &s) {
  return build_parsed_json(s.data(), s.length(), false);
}

} // namespace simdjson
#endif
