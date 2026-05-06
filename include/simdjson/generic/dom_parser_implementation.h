#ifndef SIMDJSON_GENERIC_DOM_PARSER_IMPLEMENTATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_DOM_PARSER_IMPLEMENTATION_H
#include "simdjson/generic/base.h"
#include "simdjson/internal/dom_parser_implementation.h"

#include <algorithm>
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
  internal::allocated_buffer<open_container> open_containers{};
  /** Whether each open container is a [ or { */
  internal::allocated_buffer<bool> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  inline explicit dom_parser_implementation(simdjson::allocator& alloc) noexcept;
  inline dom_parser_implementation(dom_parser_implementation &&other) noexcept;
  inline dom_parser_implementation &operator=(dom_parser_implementation &&other) noexcept;
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation &operator=(const dom_parser_implementation &) = delete;

  simdjson_warn_unused error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage1(const uint8_t *buf, size_t len, stage1_mode partial) noexcept final;
  simdjson_warn_unused error_code stage2(dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage2_next(dom::document &doc) noexcept final;
  simdjson_warn_unused uint8_t *parse_string(const uint8_t *src, uint8_t *dst, bool allow_replacement) const noexcept final;
  simdjson_warn_unused uint8_t *parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept final;
  inline simdjson_warn_unused error_code set_capacity(size_t capacity) final;
  inline simdjson_warn_unused error_code set_max_depth(size_t max_depth) final;
private:
  simdjson_inline simdjson_warn_unused error_code set_capacity_stage1(size_t capacity);

};

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

inline dom_parser_implementation::dom_parser_implementation(simdjson::allocator& alloc) noexcept
    : internal::dom_parser_implementation{alloc} {}
inline dom_parser_implementation::dom_parser_implementation(dom_parser_implementation &&other) noexcept = default;
inline dom_parser_implementation &dom_parser_implementation::operator=(dom_parser_implementation &&other) noexcept = default;

// Leaving these here so they can be inlined if so desired
inline simdjson_warn_unused error_code dom_parser_implementation::set_capacity(size_t capacity) {
  if(capacity > SIMDJSON_MAXSIZE_BYTES) { return CAPACITY; }
  // Stage 1 index output
  size_t rounded_capacity = SIMDJSON_ROUNDUP_N(capacity, 64);
  if(rounded_capacity + 9 < rounded_capacity) {
    return CAPACITY; // overflow, only happen on legacy 32-bit systems with very large capacity
  }
  size_t max_structures = rounded_capacity + 9;

  // Allocate into a local first: if the allocator throws, existing state is
  // preserved (strong exception safety).
  auto new_structural_indexes = internal::make_allocated_buffer<uint32_t>(max_structures, *_allocator);
  if (!new_structural_indexes) { return MEMALLOC; }
  new_structural_indexes[0] = 0;

  structural_indexes = std::move(new_structural_indexes);
  n_structural_indexes = 0;
  // Derive _capacity from the buffer's actual size so any allocator
  // over-allocation is exposed via parser.capacity().
  _capacity = SIMDJSON_ROUNDDOWN_N(structural_indexes.capacity() - 9, 64);
  return SUCCESS;
}

inline simdjson_warn_unused error_code dom_parser_implementation::set_max_depth(size_t max_depth) {
  if(max_depth > SIMDJSON_MAX_DEPTH) { return CAPACITY; }
  // Stage 2 stacks. Allocate into locals first: if either allocation throws,
  // existing state is preserved (strong exception safety).
  auto new_open_containers = internal::make_allocated_buffer<open_container>(max_depth, *_allocator);
  auto new_is_array = internal::make_allocated_buffer<bool>(max_depth, *_allocator);
  if (!new_open_containers || !new_is_array) { return MEMALLOC; }

  open_containers = std::move(new_open_containers);
  is_array = std::move(new_is_array);
  // Pin to the smaller of the two buffers. Both are guaranteed >= max_depth,
  // so this is always >= the requested value.
  _max_depth = std::min(open_containers.capacity(), is_array.capacity());
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_DOM_PARSER_IMPLEMENTATION_H
