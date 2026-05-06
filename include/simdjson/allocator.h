#ifndef SIMDJSON_ALLOCATOR_H
#define SIMDJSON_ALLOCATOR_H

#include <cstddef>

namespace simdjson {

/**
 * Result of a successful allocation. Models `std::allocation_result<void*>`
 * from C++23 (see `std::allocator::allocate_at_least`).
 *
 * `size` is the number of bytes actually handed out by the allocator and
 * must be passed back unchanged to `allocator::deallocate`. It may exceed
 * the size originally requested; callers may use the full returned range.
 */
struct allocation_result {
  void* ptr;
  size_t size;
};

/**
 * Abstract base class for custom allocators.
 *
 * Subclass this and pass an instance to parser constructors to control where
 * simdjson obtains memory. The allocator must outlive every parser that
 * references it.
 *
 * For wrapping an existing C++ standard-`Allocator`-conforming type (such as
 * `std::allocator<T>` or `std::pmr::polymorphic_allocator<T>`), include
 * `simdjson/allocator_adapter.h` and use `simdjson::allocator_adapter<Alloc>`
 * instead of subclassing this interface directly.
 */
class allocator {
public:
  virtual ~allocator() = default;

  /**
   * Allocate AT LEAST `size` bytes. Return `{nullptr, 0}` on failure, OR throw.
   *
   * The returned `allocation_result::size` is the true number of bytes
   * handed out; it may be greater than `size` if the allocator operates on
   * buckets or rounds up to a slab size. Callers may use the full returned
   * range and must pass the returned `size` back to `deallocate` unchanged.
   *
   * Exceptions are propagated to the caller, simdjson will
   * not catch them.
   * The nullptr-returning failure mode is mapped to `MEMALLOC` errors or
   * `is_valid = false` (on the builder).
   *
   * Alignment: returned memory must be suitably aligned for any fundamental
   * type, i.e. at least `alignof(std::max_align_t)` (equivalently
   * `__STDCPP_DEFAULT_NEW_ALIGNMENT__` in C++17+). This matches the guarantee
   * of `malloc`, `::operator new`, and all mainstream pool allocators (jemalloc,
   * tcmalloc, mimalloc, PMR).
   */
  virtual allocation_result allocate(size_t size) = 0;

  /**
   * Deallocate memory previously returned by `allocate()`.
   * `size` must equal `allocation_result::size` from the corresponding
   * `allocate()` call`.
   */
  virtual void deallocate(void* ptr, size_t size) noexcept = 0;
};

/**
 * Returns a reference to a default allocator singleton which uses `new[]` / `delete[]`.
 */
allocator& get_default_allocator() noexcept;

} // namespace simdjson
#endif // SIMDJSON_ALLOCATOR_H
