#include "simdjson.h"
#include "isadetection.h"
#include "simdprune_tables.h"

#include <initializer_list>

// Static array of known implementations. We're hoping these get baked into the executable
// without requiring a static initializer.

#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "haswell/implementation.h"
namespace simdjson::internal { const haswell::implementation haswell_singleton{}; }
#endif // SIMDJSON_IMPLEMENTATION_HASWELL

#if SIMDJSON_IMPLEMENTATION_WESTMERE
#include "westmere/implementation.h"
namespace simdjson::internal { const westmere::implementation westmere_singleton{}; }
#endif // SIMDJSON_IMPLEMENTATION_WESTMERE

#if SIMDJSON_IMPLEMENTATION_ARM64
#include "arm64/implementation.h"
namespace simdjson::internal { const arm64::implementation arm64_singleton{}; }
#endif // SIMDJSON_IMPLEMENTATION_ARM64

#if SIMDJSON_IMPLEMENTATION_FALLBACK
#include "fallback/implementation.h"
namespace simdjson::internal { const fallback::implementation fallback_singleton{}; }
#endif // SIMDJSON_IMPLEMENTATION_FALLBACK

namespace simdjson::internal {

constexpr const std::initializer_list<const implementation *> available_implementation_pointers {
#if SIMDJSON_IMPLEMENTATION_HASWELL
  &haswell_singleton,
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
  &westmere_singleton,
#endif
#if SIMDJSON_IMPLEMENTATION_ARM64
  &arm64_singleton,
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
  &fallback_singleton,
#endif
}; // available_implementation_pointers

// So we can return UNSUPPORTED_ARCHITECTURE from the parser when there is no support
class unsupported_implementation final : public implementation {
public:
  WARN_UNUSED error_code parse(const uint8_t *, size_t, parser &) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }
  WARN_UNUSED error_code minify(const uint8_t *, size_t, uint8_t *, size_t &) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }
  WARN_UNUSED error_code stage1(const uint8_t *, size_t, parser &, bool) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }
  WARN_UNUSED error_code stage2(const uint8_t *, size_t, parser &) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }
  WARN_UNUSED error_code stage2(const uint8_t *, size_t, parser &, size_t &) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }

  unsupported_implementation() : implementation("unsupported", "Unsupported CPU (no detected SIMD instructions)", 0) {}
};

const unsupported_implementation unsupported_singleton{};

size_t available_implementation_list::size() const noexcept {
  return internal::available_implementation_pointers.size();
}
const implementation * const *available_implementation_list::begin() const noexcept {
  return internal::available_implementation_pointers.begin();
}
const implementation * const *available_implementation_list::end() const noexcept {
  return internal::available_implementation_pointers.end();
}
const implementation *available_implementation_list::detect_best_supported() const noexcept {
  // They are prelisted in priority order, so we just go down the list
  uint32_t supported_instruction_sets = detect_supported_architectures();
  for (const implementation *impl : internal::available_implementation_pointers) {
    uint32_t required_instruction_sets = impl->required_instruction_sets();
    if ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets) { return impl; }
  }
  return &unsupported_singleton;
}

const implementation *detect_best_supported_implementation_on_first_use::set_best() const noexcept {
  return active_implementation = available_implementations.detect_best_supported();
}

} // namespace simdjson::internal
