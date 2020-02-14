#ifndef SIMDJSON_DOCUMENT_H
#define SIMDJSON_DOCUMENT_H

#include <cstring>
#include <memory>
#include "simdjson/common_defs.h"
#include "simdjson/simdjson.h"
#include "simdjson/padded_string.h"

#define JSON_VALUE_MASK 0xFFFFFFFFFFFFFF
#define DEFAULT_MAX_DEPTH 1024 // a JSON document with a depth exceeding 1024 is probably de facto invalid

namespace simdjson {

template<size_t max_depth> class document_iterator;

class document {
public:
  // create a document container with zero capacity, parser will allocate capacity as needed
  document()=default;
  ~document()=default;

  // this is a move only class
  document(document &&p) = default;
  document(const document &p) = delete;
  document &operator=(document &&o) = default;
  document &operator=(const document &o) = delete;

  using iterator = document_iterator<DEFAULT_MAX_DEPTH>;

  //
  // Tell whether this document has been parsed, or is just empty.
  //
  bool is_initialized() {
    return tape && string_buf;
  }

  // print the json to std::ostream (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  WARN_UNUSED
  bool print_json(std::ostream &os, size_t max_depth=DEFAULT_MAX_DEPTH) const;
  WARN_UNUSED
  bool dump_raw_tape(std::ostream &os) const;

  class parser;

  //
  // Parse a JSON document.
  //
  // If you will be parsing more than one JSON document, it's recommended to create a
  // document::parser object instead, keeping internal buffers around for efficiency reasons.
  //
  // Throws invalid_json if the JSON is invalid.
  //
  static document parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true);
  static document parse(const char *buf, size_t len, bool realloc_if_needed = true) {
      return parse((const uint8_t *)buf, len, realloc_if_needed);
  }
  static document parse(const std::string &s, bool realloc_if_needed = true) {
      return parse(s.data(), s.length(), realloc_if_needed);
  }
  static document parse(const padded_string &s) {
      return parse(s.data(), s.length(), false);
  }

  //
  // Parse a JSON document.
  //
  // If you will be parsing more than one JSON document, it's recommended to create a
  // document::parser object instead, keeping internal buffers around for efficiency reasons.
  //
  // Returns != SUCCESS if the JSON is invalid.
  //
  static WARN_UNUSED error_code try_parse(const uint8_t *buf, size_t len, document &dst, bool realloc_if_needed = true) noexcept;
  static WARN_UNUSED error_code try_parse(const char *buf, size_t len, document &dst, bool realloc_if_needed = true) {
      return try_parse((const uint8_t *)buf, len, dst, realloc_if_needed);
  }
  static WARN_UNUSED error_code try_parse(const std::string &s, document &dst, bool realloc_if_needed = true) {
      return try_parse(s.data(), s.length(), dst, realloc_if_needed);
  }
  static WARN_UNUSED error_code try_parse(const padded_string &s, document &dst) {
      return try_parse(s.data(), s.length(), dst, false);
  }

  std::unique_ptr<uint64_t[]> tape;
  std::unique_ptr<uint8_t[]> string_buf;// should be at least byte_capacity

private:
  bool set_capacity(size_t len);
};

} // namespace simdjson

#include "simdjson/document/parser.h"
#include "simdjson/document/iterator.h"

// Implementations
namespace simdjson {

inline WARN_UNUSED document document::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) {
  document::parser parser;
  if (!parser.allocate_capacity(len)) {
    throw invalid_json(parser.error = MEMALLOC);
  }
  return parser.parse_new(buf, len, realloc_if_needed);
}

inline WARN_UNUSED error_code document::try_parse(const uint8_t *buf, size_t len, document &dst, bool realloc_if_needed) noexcept {
  document::parser parser;
  if (!parser.allocate_capacity(len)) {
    return parser.error = MEMALLOC;
  }
  return parser.try_parse_into(buf, len, dst, realloc_if_needed);
}

} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_H