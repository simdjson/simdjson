#ifndef SIMDJSON_ALLOCATOR_ADAPTER_H
#define SIMDJSON_ALLOCATOR_ADAPTER_H

#include "simdjson/allocator.h"
#include "simdjson/compiler_check.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

#if SIMDJSON_SUPPORTS_CONCEPTS
#include <concepts>
namespace simdjson::concepts {
/**
 * Models the C++ standard `Allocator` named requirement, as used by
 * `simdjson::allocator_adapter`. We only check what the adapter actually
 * uses on the byte-rebound allocator; any well-formed `Allocator` satisfies
 * those checks.
 */
template <typename A>
concept cpp_allocator =
  requires { typename std::allocator_traits<A>::template rebind_alloc<uint8_t>; } &&
  requires(typename std::allocator_traits<A>::template rebind_alloc<uint8_t> a,
           std::size_t n, uint8_t* p) {
    { std::allocator_traits<decltype(a)>::allocate(a, n) } -> std::convertible_to<uint8_t*>;
    std::allocator_traits<decltype(a)>::deallocate(a, p, n);
  };
} // namespace simdjson::concepts
#endif

namespace simdjson {

/**
 * Adapter that exposes a C++ `Allocator (e.g. `std::allocator<T>`,
 * `std::pmr::polymorphic_allocator<T>`, or a third-party pool's allocator)
 * as a `simdjson::allocator`..
 *
 * When `__cpp_lib_allocate_at_least` is available (C++23), `allocate()`
 * forwards to `std::allocator_traits::allocate_at_least` so over-allocation
 * from the underlying allocator is surfaced to simdjson as usable capacity.
 * Otherwise it falls back to `allocate(n)` and reports `{p, n}`.
 *
 * Alignment: `simdjson::allocator` requires `>= alignof(std::max_align_t)`
 * alignment. `std::allocator<uint8_t>` returns `::operator new`-aligned
 * memory (>= max_align_t), so it is always safe. For
 * `std::pmr::polymorphic_allocator<uint8_t>` the rebound allocator only
 * requests `alignof(uint8_t) == 1`; callers must use a memory resource
 * that returns sufficiently aligned memory (all standard-library resources
 * and mainstream pool allocators do).
 */
template <class Alloc>
class allocator_adapter final : public allocator {
  #if SIMDJSON_SUPPORTS_CONCEPTS
  static_assert(concepts::cpp_allocator<Alloc>, "Alloc must satisfy the C++ standard Allocator named requirement");
  #endif
  using byte_alloc  = typename std::allocator_traits<Alloc>::template rebind_alloc<uint8_t>;
  using byte_traits = std::allocator_traits<byte_alloc>;

  byte_alloc inner{};

public:
  allocator_adapter() = default;
  explicit allocator_adapter(const Alloc& a) : inner(a) {}
  explicit allocator_adapter(Alloc&& a) : inner(std::move(a)) {}

  allocation_result allocate(size_t size) override {
#if defined(__cpp_lib_allocate_at_least)
    auto r = byte_traits::allocate_at_least(inner, size);
    return { r.ptr, r.count };
#else
    return { byte_traits::allocate(inner, size), size };
#endif
  }

  void deallocate(void* ptr, size_t size) noexcept override {
    byte_traits::deallocate(inner, static_cast<uint8_t*>(ptr), size);
  }
};

#if SIMDJSON_CPLUSPLUS17
// Deduction guide (CTAD is a C++17 feature)
template <class A>
allocator_adapter(A) -> allocator_adapter<A>;
#endif

} // namespace simdjson

#endif // SIMDJSON_ALLOCATOR_ADAPTER_H
