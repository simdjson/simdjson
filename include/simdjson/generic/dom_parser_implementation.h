#ifndef SIMDJSON_GENERIC_DOM_PARSER_IMPLEMENTATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_DOM_PARSER_IMPLEMENTATION_H
#include "simdjson/generic/base.h"
#include "simdjson/internal/dom_parser_implementation.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
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

  inline dom_parser_implementation() noexcept;
  inline dom_parser_implementation(dom_parser_implementation &&other) noexcept;
  inline dom_parser_implementation &operator=(dom_parser_implementation &&other) noexcept;
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation &operator=(const dom_parser_implementation &) = delete;

  simdjson_warn_unused error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage1(const uint8_t *buf, size_t len, stage1_mode partial) noexcept final;
  simdjson_warn_unused error_code stage2(dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage2_next(dom::document &doc) noexcept final;
  stage2_next_function_t stage2_next_function(bool bounded) const noexcept final;
  simdjson_warn_unused uint8_t *parse_string(const uint8_t *src, uint8_t *dst, bool allow_replacement) const noexcept final;
  simdjson_warn_unused uint8_t *parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept final;
  inline simdjson_warn_unused error_code set_capacity(size_t capacity) noexcept final;
  inline simdjson_warn_unused error_code set_max_depth(size_t max_depth) noexcept final;
private:
  simdjson_inline simdjson_warn_unused error_code set_capacity_stage1(size_t capacity);

};

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

inline dom_parser_implementation::dom_parser_implementation() noexcept = default;
inline dom_parser_implementation::dom_parser_implementation(dom_parser_implementation &&other) noexcept = default;
inline dom_parser_implementation &dom_parser_implementation::operator=(dom_parser_implementation &&other) noexcept = default;

// Leaving these here so they can be inlined if so desired
inline simdjson_warn_unused error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  if(capacity > SIMDJSON_MAXSIZE_BYTES || capacity > SIZE_MAX - 63) { return CAPACITY; }
  // Stage 1 index output
  size_t rounded_capacity = SIMDJSON_ROUNDUP_N(capacity, 64);
  if(rounded_capacity + 9 < rounded_capacity) {
    return CAPACITY; // overflow, only happen on legacy 32-bit systems with very large capacity
  }
  size_t max_structures = rounded_capacity + 9;
  if(max_structures > SIZE_MAX / sizeof(uint32_t)) { return CAPACITY; }
  // Allocate before releasing the current buffer so that, on failure, the
  // implementation keeps its previous buffer and capacity. Otherwise a parser
  // that still believes it has room skips reallocation and stage 1 rejects
  // every document with CAPACITY.
  std::unique_ptr<uint32_t[]> new_structural_indexes(new (std::nothrow) uint32_t[max_structures]);
  if (!new_structural_indexes) { return MEMALLOC; }
  structural_indexes = std::move(new_structural_indexes);
  structural_indexes[0] = 0;
  n_structural_indexes = 0;

  _capacity = capacity;
  return SUCCESS;
}

inline simdjson_warn_unused error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  if(max_depth == 0 || max_depth > SIMDJSON_MAX_DEPTH || max_depth > SIZE_MAX / sizeof(open_container)) { return CAPACITY; }
  // Stage 2 stacks. Allocate both before touching the parser so that, on
  // failure, it keeps its previous stacks and depth: a parser whose depth does
  // not match its stacks would crash in stage 2.
  std::unique_ptr<open_container[]> new_open_containers(new (std::nothrow) open_container[max_depth]);
  std::unique_ptr<bool[]> new_is_array(new (std::nothrow) bool[max_depth]);
  if (!new_is_array || !new_open_containers) { return MEMALLOC; }
  open_containers = std::move(new_open_containers);
  is_array = std::move(new_is_array);
  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_DOM_PARSER_IMPLEMENTATION_H