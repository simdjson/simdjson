#ifndef SIMDJSON_DOCUMENT_H
#define SIMDJSON_DOCUMENT_H

#include <cstring>
#include <memory>
#include "simdjson/common_defs.h"
#include "simdjson/simdjson.h"

#define JSON_VALUE_MASK 0xFFFFFFFFFFFFFF

#define DEFAULT_MAX_DEPTH                                                      \
  1024 // a JSON document with a depth exceeding 1024 is probably de facto
       // invalid

namespace simdjson {

class document {
public:
  // create a ParsedJson container with zero capacity, call allocate_capacity to
  // allocate memory
  document()=default;
  ~document()=default;

  // this is a move only class
  document(document &&p) = default;
  document(const document &p) = delete;
  document &operator=(document &&o) = default;
  document &operator=(const document &o) = delete;

  static document parse(const uint8_t *buf, size_t len);

  // returns true if the document parsed was valid
  bool is_valid() const;

  // return an error code corresponding to the last parsing attempt, see
  // simdjson.h will return simdjson::UNITIALIZED if no parsing was attempted
  int get_error_code() const;

  // if needed, allocate memory so that the object is able to process JSON
  // documents having up to len bytes and max_depth "depth"
  WARN_UNUSED
  bool allocate_capacity(size_t len, size_t max_depth = DEFAULT_MAX_DEPTH);

  // return the string equivalent of "get_error_code"
  std::string get_error_message() const;

  // deallocate memory and set capacity to zero, called automatically by the
  // destructor
  void deallocate();

  // print the json to std::ostream (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  WARN_UNUSED
  bool print_json(std::ostream &os) const;
  WARN_UNUSED
  bool dump_raw_tape(std::ostream &os) const;

  template <size_t max_depth> class iterator;
  using Iterator = document::iterator<DEFAULT_MAX_DEPTH>;

  size_t byte_capacity{0}; // indicates how many bits are meant to be supported

  size_t depth_capacity{0}; // how deep we can go
  size_t tape_capacity{0};
  size_t string_capacity{0};
  uint32_t current_loc{0};
  uint32_t n_structural_indexes{0};

  std::unique_ptr<uint32_t[]> structural_indexes;

  std::unique_ptr<uint64_t[]> tape;
  std::unique_ptr<uint32_t[]> containing_scope_offset;

#ifdef SIMDJSON_USE_COMPUTED_GOTO
  std::unique_ptr<void*[]> ret_address;
#else
  std::unique_ptr<char[]> ret_address;
#endif

  std::unique_ptr<uint8_t[]> string_buf;// should be at least byte_capacity
  uint8_t *current_string_buf_loc;
  bool valid{false};
  int error_code{simdjson::UNINITIALIZED};
};

} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_H