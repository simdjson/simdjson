#include "simdjson.h"
#include "isadetection.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {

// expectation: sizeof(open_container) = 64/8.
struct open_container {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct open_container

static_assert(sizeof(open_container) == 64/8, "Open container must be 64 bits");

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /** Tape location of each open { or [ */
  std::unique_ptr<open_container[]> open_containers{};
  /** Whether each open container is a [ or { */
  std::unique_ptr<bool[]> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  simdjson_really_inline dom_parser_implementation();
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation & operator=(const dom_parser_implementation &) = delete;

  SIMDJSON_WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  SIMDJSON_WARN_UNUSED error_code check_for_unclosed_array() noexcept;
  SIMDJSON_WARN_UNUSED error_code stage2(dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code stage2_next(dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code set_capacity(size_t capacity) noexcept final;
  SIMDJSON_WARN_UNUSED error_code set_max_depth(size_t max_depth) noexcept final;
};

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#include "generic/stage1/allocate.h"
#include "generic/stage2/allocate.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {

simdjson_really_inline dom_parser_implementation::dom_parser_implementation() {}

// Leaving these here so they can be inlined if so desired
SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  error_code err = stage1::allocate::set_capacity(*this, capacity);
  if (err) { _capacity = 0; return err; }
  _capacity = capacity;
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  error_code err = stage2::allocate::set_max_depth(*this, max_depth);
  if (err) { _max_depth = 0; return err; }
  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
