#ifndef SIMDJSON_HASWELL_DOM_PARSER_IMPLEMENTATION_H
#define SIMDJSON_HASWELL_DOM_PARSER_IMPLEMENTATION_H

#include "simdjson.h"
#include "isadetection.h"
namespace simdjson {
namespace haswell {

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  const uint8_t *buf{}; // Buffer passed to stage 1
  size_t len{0}; // Length passed to stage 1
  dom::document *doc{}; // Document passed to stage 2

  really_inline dom_parser_implementation();
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation & operator=(const dom_parser_implementation &) = delete;
  
  WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, bool streaming) noexcept final;
  WARN_UNUSED error_code stage2(dom::document &doc) noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, dom::document &doc, size_t &next_json) noexcept final;
  WARN_UNUSED error_code set_capacity(size_t capacity) noexcept final;
  WARN_UNUSED error_code set_max_depth(size_t max_depth) noexcept final;
};

#include "generic/stage1/allocate.h"
#include "generic/stage2/allocate.h"

really_inline dom_parser_implementation::dom_parser_implementation() {}

// Leaving these here so they can be inlined if so desired
WARN_UNUSED error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  error_code err = stage1::allocate::set_capacity(*this, capacity);
  if (err) { _capacity = 0; return err; }
  _capacity = capacity;
  return SUCCESS;
}

WARN_UNUSED error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  error_code err = stage2::allocate::set_max_depth(*this, max_depth);
  if (err) { _max_depth = 0; return err; }
  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace haswell
} // namespace simdjson

#endif // SIMDJSON_HASWELL_DOM_PARSER_IMPLEMENTATION_H