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

  //
  // Parse a JSON document.
  //
  // If you will be parsing more than one JSON document, it's recommended to create a
  // document::parser object instead, keeping internal buffers around for efficiency reasons.
  //
  // Throws invalid_json if the JSON is invalid.
  //
  static document parse(const uint8_t *buf, size_t len);
  static document parse(const char *buf, size_t len) {
      return parse((const uint8_t *)buf, len);
  }
  static document parse(const padded_string &s) {
      return parse(s.data(), s.length());
  }

  //
  // Parse a JSON document.
  //
  // If you will be parsing more than one JSON document, it's recommended to create a
  // document::parser object instead, keeping internal buffers around for efficiency reasons.
  //
  // Returns != SUCCESS if the JSON is invalid.
  //
  static ErrorValues try_parse_into(const uint8_t *buf, size_t len, document &dst) noexcept;
  static ErrorValues try_parse_into(const char *buf, size_t len, document &dst) {
      return try_parse_into((const uint8_t *)buf, len, dst);
  }
  static ErrorValues try_parse_into(const padded_string &s, document &dst) {
      return try_parse_into(s.data(), s.length(), dst);
  }

  // nested class declarations
  class parser;
  template <size_t max_depth=DEFAULT_MAX_DEPTH> class iterator;

  // print the json to std::ostream (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  WARN_UNUSED
  bool print_json(std::ostream &os, size_t max_depth=DEFAULT_MAX_DEPTH) const;
  WARN_UNUSED
  bool dump_raw_tape(std::ostream &os) const;

  std::unique_ptr<uint64_t[]> tape;
  std::unique_ptr<uint8_t[]> string_buf;// should be at least byte_capacity

private:
  void deallocate();
};

} // namespace simdjson

#include "simdjson/document/parser.h"
#include "simdjson/document/iterator.h"

#endif // SIMDJSON_DOCUMENT_H