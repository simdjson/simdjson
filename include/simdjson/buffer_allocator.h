#ifndef SIMDJSON_BUFFER_ALLOCATOR_H
#define SIMDJSON_BUFFER_ALLOCATOR_H

#include "simdjson/base.h"

#include <cstddef>
#include <new>

namespace simdjson {

/**
 * Allocates the buffers that a parser keeps between documents and that grow
 * with the largest document it parses: the structural indexes, the On-Demand
 * string buffer, and the DOM tape and string buffer.
 *
 * Both functions must be noexcept. allocate returns memory aligned at least
 * as malloc would align it, or nullptr on failure, in which case the parser
 * returns MEMALLOC and keeps its previous buffers. deallocate receives the
 * pointer, the size that was requested and the context.
 *
 * A parser allocates only when it grows, so a reused parser rarely calls the
 * allocator. The allocator, and its context, must outlive every parser and
 * document that holds memory from it.
 */
struct buffer_allocator {
  void *(*allocate)(void *context, size_t size) noexcept;
  void (*deallocate)(void *context, void *pointer, size_t size) noexcept;
  void *context;
};

namespace internal {

/**
 * @private An owning array of trivial elements, from a buffer_allocator or,
 * without one, from new[]. It frees itself through what allocated it.
 */
template <typename T> class allocated_array {
public:
  allocated_array() noexcept = default;
  allocated_array(const allocated_array &) = delete;
  allocated_array &operator=(const allocated_array &) = delete;
  allocated_array(allocated_array &&other) noexcept
      : data_{other.data_}, size_{other.size_}, allocator_{other.allocator_} {
    other.data_ = nullptr;
    other.size_ = 0;
    other.allocator_ = nullptr;
  }
  allocated_array &operator=(allocated_array &&other) noexcept {
    if (this != &other) {
      reset();
      data_ = other.data_;
      size_ = other.size_;
      allocator_ = other.allocator_;
      other.data_ = nullptr;
      other.size_ = 0;
      other.allocator_ = nullptr;
    }
    return *this;
  }
  ~allocated_array() noexcept { reset(); }

  /** Replace the content by count uninitialized elements; false on failure. */
  bool allocate(size_t count, const buffer_allocator *allocator) noexcept {
    reset();
    if (allocator) {
      if (count > size_t(-1) / sizeof(T)) {
        return false;
      }
      data_ = static_cast<T *>(allocator->allocate(allocator->context, count * sizeof(T)));
    } else {
      data_ = new (std::nothrow) T[count];
    }
    if (!data_) {
      return false;
    }
    size_ = count;
    allocator_ = allocator;
    return true;
  }
  void reset() noexcept {
    if (!data_) {
      return;
    }
    if (allocator_) {
      allocator_->deallocate(allocator_->context, data_, size_ * sizeof(T));
    } else {
      delete[] data_;
    }
    data_ = nullptr;
    size_ = 0;
    allocator_ = nullptr;
  }
  simdjson_inline T *get() const noexcept { return data_; }
  simdjson_inline T &operator[](size_t index) const noexcept { return data_[index]; }
  simdjson_inline explicit operator bool() const noexcept { return data_ != nullptr; }

private:
  T *data_{nullptr};
  size_t size_{0};
  const buffer_allocator *allocator_{nullptr};
};

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_BUFFER_ALLOCATOR_H
