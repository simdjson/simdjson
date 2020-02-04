#ifndef SIMDJSON_DOCUMENT_PARSER_H
#define SIMDJSON_DOCUMENT_PARSER_H

#include <cstring>
#include <memory>
#include "simdjson/common_defs.h"
#include "simdjson/simdjson.h"
#include "simdjson/document.h"

namespace simdjson {

class document::parser {
public:
  // create a ParsedJson container with zero capacity, call allocate_capacity to allocate memory
  parser()=default;
  ~parser()=default;

  // this is a move only class
  parser(parser &&p) = default;
  parser(const parser &p) = delete;
  parser &operator=(parser &&o) = default;
  parser &operator=(const parser &o) = delete;

//   bool parse(const uint8_t *buf, size_t len, document &doc);
//   document parse(const uint8_t *buf, size_t len);

  // if needed, allocate memory so that the object is able to process JSON
  // documents having up to len bytes and max_depth "depth"
  WARN_UNUSED
  bool allocate_capacity(size_t len, size_t max_depth = DEFAULT_MAX_DEPTH);

  // deallocate memory and set capacity to zero, called automatically by the destructor
  void deallocate();

  // this should be called when parsing (right before writing the tapes)
  void init(document &doc);

  size_t byte_capacity{0};
  size_t depth_capacity{0}; // how deep we can go
  uint32_t current_loc{0};
  uint32_t n_structural_indexes{0};

  std::unique_ptr<uint32_t[]> structural_indexes;

  std::unique_ptr<uint32_t[]> containing_scope_offset;

#ifdef SIMDJSON_USE_COMPUTED_GOTO
  std::unique_ptr<void*[]> ret_address;
#else
  std::unique_ptr<char[]> ret_address;
#endif

  uint8_t *current_string_buf_loc;
};

} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_PARSER_H