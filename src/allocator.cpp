#ifndef SIMDJSON_SRC_ALLOCATOR_CPP
#define SIMDJSON_SRC_ALLOCATOR_CPP

#include <base.h>
#include <simdjson/allocator.h>

#include <new>

namespace simdjson {

/**
 * The built-in default allocator. Uses `new[]` / `delete[]`.
 */
class default_allocator final : public allocator {
public:
  allocation_result allocate(size_t size) override {
    return { new (std::nothrow) uint8_t[size], size };
  }
  void deallocate(void* ptr, size_t /*size*/) noexcept override {
    delete[] static_cast<uint8_t*>(ptr);
  }
};

allocator& get_default_allocator() noexcept {
  static default_allocator instance;
  return instance;
}

} // namespace simdjson

#endif // SIMDJSON_SRC_ALLOCATOR_CPP
