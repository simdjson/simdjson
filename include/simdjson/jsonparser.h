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
#ifdef _MSC_VER
#include <windows.h>
// must be included after windows.h
#include <sysinfoapi.h>
#else
#include <unistd.h>
#endif

namespace simdjson {
// The function that users are expected to call is json_parse.
// We have more than one such function because we want to support several
// instruction sets.

// function pointer type for json_parse
using json_parse_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj,
                                bool realloc_if_needed);

// Pointer that holds the json_parse implementation corresponding to the
// available SIMD instruction set
extern json_parse_functype *json_parse_ptr;

// json_parse_implementation is the generic function, it is specialized for
// various architectures, e.g., as
// json_parse_implementation<Architecture::HASWELL> or
// json_parse_implementation<Architecture::ARM64>
template <Architecture T>
int json_parse_implementation(const uint8_t *buf, size_t len, ParsedJson &pj,
                              bool realloc_if_needed = true) {
  if (pj.byte_capacity < len) {
    return simdjson::CAPACITY;
  }
  bool reallocated = false;
  if (realloc_if_needed) {
#if ALLOW_SAME_PAGE_BUFFER_OVERRUN
    // realloc is needed if the end of the memory crosses a page
#ifdef _MSC_VER
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    long page_size = sysInfo.dwPageSize;
#else
    long page_size = sysconf(_SC_PAGESIZE);
#endif
    //////////////
    // We want to check that buf + len - 1 and buf + len - 1 + SIMDJSON_PADDING
    // are in the same page.
    // That is, we want to check that
    // (buf + len - 1) / page_size == (buf + len - 1 + SIMDJSON_PADDING) /
    // page_size That's true if (buf + len - 1) % page_size + SIMDJSON_PADDING <
    // page_size.
    ///////////
    if ((reinterpret_cast<uintptr_t>(buf + len - 1) % page_size) +
            SIMDJSON_PADDING <
        static_cast<uintptr_t>(page_size)) {
#else // SIMDJSON_SAFE_SAME_PAGE_READ_OVERRUN
    if (true) { // if not SIMDJSON_SAFE_SAME_PAGE_READ_OVERRUN, we always
                // reallocate
#endif
      const uint8_t *tmp_buf = buf;
      buf = (uint8_t *)allocate_padded_buffer(len);
      if (buf == NULL)
        return simdjson::MEMALLOC;
      memcpy((void *)buf, tmp_buf, len);
      reallocated = true;
    } // if (true) OR if ( (reinterpret_cast<uintptr_t>(buf + len - 1) %
      // page_size ) + SIMDJSON_PADDING < static_cast<uintptr_t>(page_size) ) {
  }   // if(realloc_if_needed) {
  int stage1_is_ok = simdjson::find_structural_bits<T>(buf, len, pj);
  if (stage1_is_ok != simdjson::SUCCESS) {
    pj.error_code = stage1_is_ok;
    return pj.error_code;
  }
  int res = unified_machine<T>(buf, len, pj);
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
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// You can also check validity by calling pj.is_valid(). The same ParsedJson can
// be reused for other documents.
//
// If realloc_if_needed is true (default) then a temporary buffer is created
// when needed during processing (a copy of the input string is made). The input
// buf should be readable up to buf + len + SIMDJSON_PADDING if
// realloc_if_needed is false, all bytes at and after buf + len  are ignored
// (can be garbage). The ParsedJson object can be reused.

inline int json_parse(const uint8_t *buf, size_t len, ParsedJson &pj,
                      bool realloc_if_needed = true) {
  return json_parse_ptr(buf, len, pj, realloc_if_needed);
}

// Parse a document found in buf.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// You can also check validity
// by calling pj.is_valid(). The same ParsedJson can be reused for other
// documents.
//
// If realloc_if_needed is true (default) then a temporary buffer is created
// when needed during processing (a copy of the input string is made). The input
// buf should be readable up to buf + len + SIMDJSON_PADDING  if
// realloc_if_needed is false, all bytes at and after buf + len  are ignored
// (can be garbage). The ParsedJson object can be reused.
inline int json_parse(const char *buf, size_t len, ParsedJson &pj,
                      bool realloc_if_needed = true) {
  return json_parse_ptr(reinterpret_cast<const uint8_t *>(buf), len, pj,
                        realloc_if_needed);
}

// We do not want to allow implicit conversion from C string to std::string.
int json_parse(const char *buf, ParsedJson &pj) = delete;

// Parse a document found in in string s.
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// A temporary buffer is created when needed during processing
// (a copy of the input string is made).
inline int json_parse(const std::string &s, ParsedJson &pj) {
  return json_parse(s.data(), s.length(), pj, true);
}

// Parse a document found in in string s.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// You can also check validity
// by calling pj.is_valid(). The same ParsedJson can be reused for other
// documents.
inline int json_parse(const padded_string &s, ParsedJson &pj) {
  return json_parse(s.data(), s.length(), pj, false);
}

// Build a ParsedJson object. You can check validity
// by calling pj.is_valid(). This does the memory allocation needed for
// ParsedJson. If realloc_if_needed is true (default) then a temporary buffer is
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
ParsedJson build_parsed_json(const uint8_t *buf, size_t len,
                             bool realloc_if_needed = true);

WARN_UNUSED
// Build a ParsedJson object. You can check validity
// by calling pj.is_valid(). This does the memory allocation needed for
// ParsedJson. If realloc_if_needed is true (default) then a temporary buffer is
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
inline ParsedJson build_parsed_json(const char *buf, size_t len,
                                    bool realloc_if_needed = true) {
  return build_parsed_json(reinterpret_cast<const uint8_t *>(buf), len,
                           realloc_if_needed);
}

// We do not want to allow implicit conversion from C string to std::string.
ParsedJson build_parsed_json(const char *buf) = delete;

// Parse a document found in in string s.
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)). Return SUCCESS (an integer = 0) in case of a
// success. You can also check validity by calling pj.is_valid(). The same
// ParsedJson can be reused for other documents.
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
inline ParsedJson build_parsed_json(const std::string &s) {
  return build_parsed_json(s.data(), s.length(), true);
}

// Parse a document found in in string s.
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)). Return SUCCESS (an integer = 0) in case of a
// success. You can also check validity by calling pj.is_valid(). The same
// ParsedJson can be reused for other documents.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// This is a convenience function which calls json_parse.
WARN_UNUSED
inline ParsedJson build_parsed_json(const padded_string &s) {
  return build_parsed_json(s.data(), s.length(), false);
}
} // namespace simdjson
#endif
