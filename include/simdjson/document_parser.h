#ifndef SIMDJSON_DOCUMENT_PARSER_H
#define SIMDJSON_DOCUMENT_PARSER_H

#include <cstring>
#include <memory>
#include "simdjson/common_defs.h"
#include "simdjson/simdjson.h"
#include "simdjson/document.h"
#include "simdjson/padded_string.h"

namespace simdjson {

class document::parser {
public:
  //
  // Create a JSON parser with zero capacity. Call allocate_capacity() to initialize it.
  //
  parser()=default;
  ~parser()=default;

  // this is a move only class
  parser(document::parser &&p) = default;
  parser(const document::parser &p) = delete;
  parser &operator=(document::parser &&o) = default;
  parser &operator=(const document::parser &o) = delete;

  //
  // Parse a JSON document and return a reference to it.
  //
  // The JSON document still lives in the parser: this is the most efficient way to parse JSON
  // documents because it reuses the same buffers, but you *must* use the document before you
  // destroy the parser or call parse() again.
  //
  // Throws invalid_json if the JSON is invalid.
  //
  inline doc_ref_result parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true);
  inline doc_ref_result parse(const char *buf, size_t len, bool realloc_if_needed = true);
  inline doc_ref_result parse(const std::string &s, bool realloc_if_needed = true);
  inline doc_ref_result parse(const padded_string &s);

  //
  // Current capacity: the largest document this parser can support without reallocating.
  //
  size_t capacity() {
    return _capacity;
  }

  //
  // The maximum level of nested object and arrays supported by this parser.
  //
  size_t max_depth() {
    return _max_depth;
  }

  // if needed, allocate memory so that the object is able to process JSON
  // documents having up to capacity bytes and max_depth "depth"
  WARN_UNUSED bool allocate_capacity(size_t capacity, size_t max_depth = DEFAULT_MAX_DEPTH) {
    return set_capacity(capacity) && set_max_depth(max_depth);
  }

  // type aliases for backcompat
  using Iterator = document::iterator;
  using InvalidJSON = invalid_json;
  class doc_result;

  // Next location to write to in the tape
  uint32_t current_loc{0};

  // structural indices passed from stage 1 to stage 2
  uint32_t n_structural_indexes{0};
  std::unique_ptr<uint32_t[]> structural_indexes;

  // location and return address of each open { or [
  std::unique_ptr<uint32_t[]> containing_scope_offset;
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  std::unique_ptr<void*[]> ret_address;
#else
  std::unique_ptr<char[]> ret_address;
#endif

  // Next place to write a string
  uint8_t *current_string_buf_loc;

  bool valid{false};
  error_code error{simdjson::UNINITIALIZED};

  // Document we're writing to
  document doc;

  // returns true if the document parsed was valid
  bool is_valid() const;

  // return an error code corresponding to the last parsing attempt, see
  // simdjson.h will return simdjson::UNITIALIZED if no parsing was attempted
  int get_error_code() const;

  // return the string equivalent of "get_error_code"
  std::string get_error_message() const;

  //
  // for backcompat with ParsedJson
  //

  // print the json to std::ostream (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  WARN_UNUSED
  bool print_json(std::ostream &os) const;
  WARN_UNUSED
  bool dump_raw_tape(std::ostream &os) const;

  // this should be called when parsing (right before writing the tapes)
  void init_stage2();

  really_inline error_code on_error(error_code new_error_code) {
    error = new_error_code;
    return new_error_code;
  }
  really_inline error_code on_success(error_code success_code) {
    error = success_code;
    valid = true;
    return success_code;
  }
  really_inline bool on_start_document(uint32_t depth) {
    containing_scope_offset[depth] = current_loc;
    write_tape(0, 'r');
    return true;
  }
  really_inline bool on_start_object(uint32_t depth) {
    containing_scope_offset[depth] = current_loc;
    write_tape(0, '{');
    return true;
  }
  really_inline bool on_start_array(uint32_t depth) {
    containing_scope_offset[depth] = current_loc;
    write_tape(0, '[');
    return true;
  }
  // TODO we're not checking this bool
  really_inline bool on_end_document(uint32_t depth) {
    // write our doc.tape location to the header scope
    // The root scope gets written *at* the previous location.
    annotate_previous_loc(containing_scope_offset[depth], current_loc);
    write_tape(containing_scope_offset[depth], 'r');
    return true;
  }
  really_inline bool on_end_object(uint32_t depth) {
    // write our doc.tape location to the header scope
    write_tape(containing_scope_offset[depth], '}');
    annotate_previous_loc(containing_scope_offset[depth], current_loc);
    return true;
  }
  really_inline bool on_end_array(uint32_t depth) {
    // write our doc.tape location to the header scope
    write_tape(containing_scope_offset[depth], ']');
    annotate_previous_loc(containing_scope_offset[depth], current_loc);
    return true;
  }

  really_inline bool on_true_atom() {
    write_tape(0, 't');
    return true;
  }
  really_inline bool on_false_atom() {
    write_tape(0, 'f');
    return true;
  }
  really_inline bool on_null_atom() {
    write_tape(0, 'n');
    return true;
  }

  really_inline uint8_t *on_start_string() {
    /* we advance the point, accounting for the fact that we have a NULL
      * termination         */
    write_tape(current_string_buf_loc - doc.string_buf.get(), '"');
    return current_string_buf_loc + sizeof(uint32_t);
  }

  really_inline bool on_end_string(uint8_t *dst) {
    uint32_t str_length = dst - (current_string_buf_loc + sizeof(uint32_t));
    // TODO check for overflow in case someone has a crazy string (>=4GB?)
    // But only add the overflow check when the document itself exceeds 4GB
    // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
    memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
    // NULL termination is still handy if you expect all your strings to
    // be NULL terminated? It comes at a small cost
    *dst = 0;
    current_string_buf_loc = dst + 1;
    return true;
  }

  really_inline bool on_number_s64(int64_t value) {
    write_tape(0, 'l');
    std::memcpy(&doc.tape[current_loc], &value, sizeof(value));
    ++current_loc;
    return true;
  }
  really_inline bool on_number_u64(uint64_t value) {
    write_tape(0, 'u');
    doc.tape[current_loc++] = value;
    return true;
  }
  really_inline bool on_number_double(double value) {
    write_tape(0, 'd');
    static_assert(sizeof(value) == sizeof(doc.tape[current_loc]), "mismatch size");
    memcpy(&doc.tape[current_loc++], &value, sizeof(double));
    // doc.tape[doc.current_loc++] = *((uint64_t *)&d);
    return true;
  }

  //
  // Called before a parse is initiated.
  //
  // - Returns CAPACITY if the document is too large
  // - Returns MEMALLOC if we needed to allocate memory and could not
  //
  WARN_UNUSED error_code init_parse(size_t len);

  const document &get_document() const {
    if (!is_valid()) {
      throw invalid_json(error);
    }
    return doc;
  }

private:
  //
  // The maximum document length this parser supports.
  //
  // Buffers are large enough to handle any document up to this length.
  //
  size_t _capacity{0};

  //
  // The maximum depth (number of nested objects and arrays) supported by this parser.
  //
  // Defaults to DEFAULT_MAX_DEPTH.
  //
  size_t _max_depth{0};

  // all nodes are stored on the doc.tape using a 64-bit word.
  //
  // strings, double and ints are stored as
  //  a 64-bit word with a pointer to the actual value
  //
  //
  //
  // for objects or arrays, store [ or {  at the beginning and } and ] at the
  // end. For the openings ([ or {), we annotate them with a reference to the
  // location on the doc.tape of the end, and for then closings (} and ]), we
  // annotate them with a reference to the location of the opening
  //
  //

  // this should be considered a private function
  really_inline void write_tape(uint64_t val, uint8_t c) {
    doc.tape[current_loc++] = val | ((static_cast<uint64_t>(c)) << 56);
  }

  really_inline void annotate_previous_loc(uint32_t saved_loc, uint64_t val) {
    doc.tape[saved_loc] |= val;
  }

  //
  // Set the current capacity: the largest document this parser can support without reallocating.
  //
  // This will allocate *or deallocate* as necessary.
  //
  // Returns false if allocation fails.
  //
  WARN_UNUSED bool set_capacity(size_t capacity);

  //
  // Set the maximum level of nested object and arrays supported by this parser.
  //
  // This will allocate *or deallocate* as necessary.
  //
  // Returns false if allocation fails.
  //
  WARN_UNUSED bool set_max_depth(size_t max_depth);
};

//
// C API (json_parse and build_parsed_json) declarations
//

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


//
// Stage 1 implementation declarations
//

// Setting the streaming parameter to true allows the find_structural_bits to tolerate unclosed strings.
// The caller should still ensure that the input is valid UTF-8. If you are processing substrings,
// you may want to call on a function like trimmed_length_safe_utf8.
// A function like find_last_json_buf_idx may also prove useful.
template <architecture T = architecture::NATIVE>
int find_structural_bits(const uint8_t *buf, size_t len, document::parser &parser, bool streaming);

// Setting the streaming parameter to true allows the find_structural_bits to tolerate unclosed strings.
// The caller should still ensure that the input is valid UTF-8. If you are processing substrings,
// you may want to call on a function like trimmed_length_safe_utf8.
// A function like find_last_json_buf_idx may also prove useful.
template <architecture T = architecture::NATIVE>
int find_structural_bits(const char *buf, size_t len, document::parser &parser, bool streaming) {
  return find_structural_bits<T>((const uint8_t *)buf, len, parser, streaming);
}

template <architecture T = architecture::NATIVE>
int find_structural_bits(const uint8_t *buf, size_t len, document::parser &parser) {
  return find_structural_bits<T>(buf, len, parser, false);
}

template <architecture T = architecture::NATIVE>
int find_structural_bits(const char *buf, size_t len, document::parser &parser) {
  return find_structural_bits<T>((const uint8_t *)buf, len, parser);
}

//
// Stage 2 implementation declarations
//

template <architecture T = architecture::NATIVE>
WARN_UNUSED int
unified_machine(const uint8_t *buf, size_t len, document::parser &parser);

template <architecture T = architecture::NATIVE>
WARN_UNUSED int
unified_machine(const char *buf, size_t len, document::parser &parser) {
  return unified_machine<T>(reinterpret_cast<const uint8_t *>(buf), len, parser);
}


// Streaming
template <architecture T = architecture::NATIVE>
WARN_UNUSED int
unified_machine(const uint8_t *buf, size_t len, document::parser &parser, size_t &next_json);

template <architecture T = architecture::NATIVE>
int unified_machine(const char *buf, size_t len, document::parser &parser, size_t &next_json) {
    return unified_machine<T>(reinterpret_cast<const uint8_t *>(buf), len, parser, next_json);
}

} // namespace simdjson

//
// Inline implementation
//

#include "simdjson/document_parser.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_build_tape.h"

namespace simdjson {

inline document::doc_ref_result document::parser::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) {
  auto code = (error_code)json_parse(buf, len, *this, realloc_if_needed);
  valid = false;
  error = UNINITIALIZED;
  return document::doc_ref_result(doc, code);
}
really_inline document::doc_ref_result document::parser::parse(const char *buf, size_t len, bool realloc_if_needed) {
  return parse((const uint8_t *)buf, len, realloc_if_needed);
}
really_inline document::doc_ref_result document::parser::parse(const std::string &s, bool realloc_if_needed) {
  return parse(s.data(), s.length(), realloc_if_needed);
}
really_inline document::doc_ref_result document::parser::parse(const padded_string &s) {
  return parse(s.data(), s.length(), false);
}

inline document::doc_result document::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) {
  document::parser parser;
  if (!parser.allocate_capacity(len)) {
    return MEMALLOC;
  }
  auto [doc, error] = parser.parse(buf, len, realloc_if_needed);
  return document::doc_result((document &&)doc, error);
}
inline document::doc_result document::parse(const char *buf, size_t len, bool realloc_if_needed) {
    return parse((const uint8_t *)buf, len, realloc_if_needed);
}
inline document::doc_result document::parse(const std::string &s, bool realloc_if_needed) {
    return parse(s.data(), s.length(), realloc_if_needed);
}
inline document::doc_result document::parse(const padded_string &s) {
    return parse(s.data(), s.length(), false);
}

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

} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_PARSER_H