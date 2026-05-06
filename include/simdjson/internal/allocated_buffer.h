#ifndef SIMDJSON_INTERNAL_ALLOCATED_BUFFER_H
#define SIMDJSON_INTERNAL_ALLOCATED_BUFFER_H

#include "simdjson/allocator.h"

#include <cstddef>
#include <limits>
#include <type_traits>

namespace simdjson {
namespace internal {

/**
 * Move-only RAII wrapper for a buffer allocated through `simdjson::allocator`.
 * Replaces `std::unique_ptr<T[]>` for internal buffers.
 */
template<typename T>
class allocated_buffer {
  T* ptr_ = nullptr;
  size_t allocated_bytes_ = 0;
  /**
   * Stores the allocator as a pointer (not a reference) so that move-assign
   * can rebind it. A default-constructed (empty) buffer has `alloc_ == nullptr`;
   * that is fine because `reset()` guards on `ptr_` being non-null.
   */
  simdjson::allocator* alloc_ = nullptr;

public:
  allocated_buffer() noexcept = default;

  /**
   * Takes ownership of `ptr` (previously obtained from `alloc.allocate(allocated_bytes)`).
   */
  allocated_buffer(T* ptr, size_t allocated_bytes, simdjson::allocator& alloc) noexcept
      : ptr_(ptr), allocated_bytes_(allocated_bytes), alloc_(&alloc) {}

  ~allocated_buffer() noexcept { reset(); }

  allocated_buffer(allocated_buffer&& o) noexcept
      : ptr_(o.ptr_), allocated_bytes_(o.allocated_bytes_), alloc_(o.alloc_) {
    o.ptr_ = nullptr;
    o.allocated_bytes_ = 0;
    o.alloc_ = nullptr;
  }

  allocated_buffer& operator=(allocated_buffer&& o) noexcept {
    if (this != &o) {
      reset();
      ptr_ = o.ptr_;
      allocated_bytes_ = o.allocated_bytes_;
      alloc_ = o.alloc_;
      o.ptr_ = nullptr;
      o.allocated_bytes_ = 0;
      o.alloc_ = nullptr;
    }
    return *this;
  }

  allocated_buffer(const allocated_buffer&) = delete;
  allocated_buffer& operator=(const allocated_buffer&) = delete;

  void reset() noexcept {
    if (ptr_) {
      alloc_->deallocate(ptr_, allocated_bytes_);
    }
    ptr_ = nullptr;
    allocated_bytes_ = 0;
  }

  T* get() const noexcept { return ptr_; }
  T& operator[](size_t i) const noexcept { return ptr_[i]; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  /**
   * Number of T elements the buffer can hold. May exceed what the caller
   * originally asked for if the allocator over-allocated (bucket / slab
   * allocators, `allocate_at_least`-style behaviour).
   */
  size_t capacity() const noexcept { return allocated_bytes_ / sizeof(T); }
};

/**
 * Helper: allocate at least `count` elements of type T via the given allocator.
 * Returns an empty buffer if the allocator returned a null pointer.
 * If the allocator throws, the exception propagates to the caller unchanged --
 * simdjson never catches it.
 *
 * The returned buffer may have a larger `capacity()` than `count` if the
 * allocator over-allocated; callers interested in that extra space can query
 * `allocated_buffer<T>::capacity()`.
 */
template<typename T>
allocated_buffer<T> make_allocated_buffer(size_t count, simdjson::allocator& alloc) {
  static_assert(alignof(T) <= alignof(std::max_align_t),
      "simdjson::allocator only guarantees max_align_t alignment");
  // Guard against overflow.
  if (count > (std::numeric_limits<size_t>::max)() / sizeof(T)) { return {}; }
  allocation_result r = alloc.allocate(count * sizeof(T));
  if (!r.ptr) { return {}; }
  return allocated_buffer<T>(static_cast<T*>(r.ptr), r.size, alloc);
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_ALLOCATED_BUFFER_H
