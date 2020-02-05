#ifndef SIMDJSON_DOCUMENT_H
#define SIMDJSON_DOCUMENT_H

#include <cstring>
#include <memory>
#include "simdjson/common_defs.h"
#include "simdjson/simdjson.h"

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

  static document parse(const uint8_t *buf, size_t len);

  // nested class declarations
  template <size_t max_depth> class iterator;
  class parser;
  using Iterator = document::iterator<DEFAULT_MAX_DEPTH>;

  // returns true if the document parsed was valid
  bool is_valid() const;

  // return an error code corresponding to the last parsing attempt, see
  // simdjson.h will return simdjson::UNITIALIZED if no parsing was attempted
  int get_error_code() const;

  // return the string equivalent of "get_error_code"
  std::string get_error_message() const;

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
  bool valid{false};
  int error_code{simdjson::UNINITIALIZED};
  void deallocate();
};

} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_H