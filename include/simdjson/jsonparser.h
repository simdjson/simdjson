#ifndef SIMDJSON_JSONPARSER_H
#define SIMDJSON_JSONPARSER_H
#include "simdjson/common_defs.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/padded_string.h"
#include "simdjson/parsedjson.h"
#include "simdjson/parsedjsoniterator.h"
#include "simdjson/simdjson.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_build_tape.h"
#include <string>

namespace simdjson {
// json_parse_implementation is the generic function, it is specialized for
// various architectures, e.g., as
// json_parse_implementation<Architecture::HASWELL> or
// json_parse_implementation<Architecture::ARM64>
template <Architecture T>
int json_parse_implementation(const uint8_t *buf, size_t len, ParsedJson &pj,
                              bool realloc_if_needed = true) {
  if (pj.byte_capacity < len) {
    pj.error_code = simdjson::CAPACITY;
    return pj.error_code;
  }
  bool reallocated = false;
  if (realloc_if_needed) {
      const uint8_t *tmp_buf = buf;
      buf = (uint8_t *)allocate_padded_buffer(len);
      if (buf == NULL)
        return simdjson::MEMALLOC;
      memcpy((void *)buf, tmp_buf, len);
      reallocated = true;
  }   // if(realloc_if_needed) {
  int stage1_is_ok = simdjson::find_structural_bits<T>(buf, len, pj);
  if (stage1_is_ok != simdjson::SUCCESS) {
    if (reallocated) { // must free before we exit
      aligned_free((void *)buf);
    }
    pj.error_code = stage1_is_ok;
    return pj.error_code;
  }
  int res = unified_machine<T>(buf, len, pj);
  if (reallocated) {
    aligned_free((void *)buf);
  }
  return res;
}
namespace {

typedef struct {
  size_t last_index;
  size_t last_buf_index;
  bool has_error;
} last_index_t;

last_index_t find_last_open_brace(const uint8_t *buf, size_t len,
                                  ParsedJson &pj) {
  last_index_t answer;
  if (pj.n_structural_indexes == 0) {
    answer.has_error = true;
    return answer;
  }
  uint32_t largest = pj.n_structural_indexes - 1;
  if (pj.structural_indexes[largest] == len) {
    if (largest == 0) {
      answer.has_error = true;
      return answer;
    }
    largest--;
  }
  size_t i = largest;
  for (; i >= 1; i--) {
    char c = buf[pj.structural_indexes[i]];
    if ((c == '{') || (c == '[')) {
      break;
    }
  }
  if (i == 0) {
    answer.has_error = true;
    return answer;
  }
  answer.has_error = false;
  answer.last_index = i;
  answer.last_buf_index = pj.structural_indexes[i];
  return answer;
}
}

// interleaved_json_parse_implementation is the generic function, it is
// specialized for
// various architectures, e.g., as
// interleaved_json_parse_implementation<Architecture::HASWELL> or
// interleaved_json_parse_implementation<Architecture::ARM64>
template <Architecture T>
int interleaved_json_parse_implementation(const uint8_t *buf, size_t len,
                                          ParsedJson &pj, size_t window,
                                          bool realloc_if_needed = true) {
  if (pj.byte_capacity < len) {
    pj.error_code = simdjson::CAPACITY;
    return pj.error_code;
  }
  if (window <= 0) {
    pj.error_code = simdjson::WINDOW_ERROR;
    return pj.error_code;
  }
  bool reallocated = false;
  if (realloc_if_needed) {
    const uint8_t *tmp_buf = buf;
    buf = (uint8_t *)allocate_padded_buffer(len);
    if (buf == NULL)
      return simdjson::MEMALLOC;
    memcpy((void *)buf, tmp_buf, len);
    reallocated = true;
  } // if(realloc_if_needed) {

  size_t idx = 0;
  size_t actual_window = window > len ? len : window;
  bool last_window = false;
  if (window >= len)
    last_window = true;
  int stage1_is_ok =
      find_structural_bits<T>(buf, actual_window, pj, !last_window);
  if (stage1_is_ok != simdjson::SUCCESS) {
    pj.error_code = stage1_is_ok;
    if (reallocated) {
      aligned_free((void *)buf);
    }
    return pj.error_code;
  }
  int stage2_is_ok = unified_machine_init(buf, actual_window, pj);
  if (stage2_is_ok != simdjson::SUCCESS_AND_HAS_MORE) {
    pj.error_code = stage2_is_ok;
    if (reallocated) {
      aligned_free((void *)buf);
    }
    return pj.error_code;
  }
  while (idx + window < len) {
    actual_window = window;
    stage1_is_ok = find_structural_bits<T>(buf + idx, actual_window, pj, true);
    if (stage1_is_ok != simdjson::SUCCESS) {
      pj.error_code = stage1_is_ok;
      if (reallocated) {
        aligned_free((void *)buf);
      }
      return pj.error_code;
    }
    last_index_t last_open_brace_index =
        find_last_open_brace(buf + idx, actual_window, pj);
    if (last_open_brace_index.has_error) {
      pj.error_code = simdjson::WINDOW_ERROR;
      if (reallocated) {
        aligned_free((void *)buf);
      }
      return pj.error_code;
    }
    actual_window = last_open_brace_index.last_buf_index;
    stage2_is_ok = unified_machine_continue<T>(
        buf + idx, actual_window, pj, last_open_brace_index.last_index);
    if (stage2_is_ok != simdjson::SUCCESS_AND_HAS_MORE) {
      pj.error_code = stage2_is_ok;
      if (reallocated) {
        aligned_free((void *)buf);
      }
      return stage2_is_ok;
    }
    idx += actual_window;
  }
  if (idx < len) {
    actual_window = len - idx;
    stage1_is_ok = find_structural_bits<T>(buf + idx, actual_window, pj, false);

    if (stage1_is_ok != simdjson::SUCCESS) {
      pj.error_code = stage1_is_ok;
      if (reallocated) {
        aligned_free((void *)buf);
      }
      return pj.error_code;
    }
    stage2_is_ok =
        unified_machine_finish<T>(buf + idx, actual_window, pj);
    idx += actual_window;
  }
  pj.error_code = stage2_is_ok;
  if (reallocated) {
    aligned_free((void *)buf);
  }
  return pj.error_code;
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

int json_parse(const uint8_t *buf, size_t len, ParsedJson &pj,
               bool realloc_if_needed = true);

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
int json_parse(const char *buf, size_t len, ParsedJson &pj,
               bool realloc_if_needed = true);

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
