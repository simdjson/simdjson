#include "simdjson/portability.h"
#include "simdjson/isadetection.h"
#include "simdjson/implementation.h"
#include <initializer_list>

// Static array of known implementations. We're hoping these get baked into the executable
// without requiring a static initializer.

#ifdef IS_X86_64

#include "haswell/implementation.h"
#include "westmere/implementation.h"

namespace simdjson {
  const haswell::implementation haswell_singleton{};
  const westmere::implementation westmere_singleton{};
  constexpr const std::initializer_list<const implementation *> available_implementation_pointers { &haswell_singleton, &westmere_singleton };
}

#endif

#ifdef IS_ARM64

#include "arm64/implementation.h"

namespace simdjson {
  const arm64::implementation arm64_singleton{};
  constexpr const std::initializer_list<const implementation *> available_implementation_pointers { &arm64_singleton };
}

#endif

namespace simdjson {

// So we can return UNSUPPORTED_ARCHITECTURE from the parser when there is no support
class unsupported_implementation final : public implementation {
public:
  WARN_UNUSED virtual error_code parse(const uint8_t *, size_t, document::parser &) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }
  WARN_UNUSED error_code stage1(const uint8_t *, size_t, document::parser &, bool) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }
  WARN_UNUSED error_code stage2(const uint8_t *, size_t, document::parser &) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }
  WARN_UNUSED error_code stage2(const uint8_t *, size_t, document::parser &, size_t &) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }

  unsupported_implementation() : implementation("unsupported", "Unsupported CPU (no detected SIMD instructions)", 0) {}
};

const unsupported_implementation unsupported_singleton{};

size_t available_implementations::size() const noexcept {
  return available_implementation_pointers.size();
}
const implementation * const *available_implementations::begin() const noexcept {
  return available_implementation_pointers.begin();
}
const implementation * const *available_implementations::end() const noexcept {
  return available_implementation_pointers.end();
}
const implementation *available_implementations::detect_best_supported() const noexcept {
  // They are prelisted in priority order, so we just go down the list
  uint32_t supported_instruction_sets = detect_supported_architectures();
  for (const implementation *impl : available_implementation_pointers) {
    uint32_t required_instruction_sets = impl->required_instruction_sets();
    if ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets) { return impl; }
  }
  return &unsupported_singleton;
}

const implementation *detect_best_supported_implementation_on_first_use::set_best() const noexcept {
  return active_implementation = available_implementations().detect_best_supported();
}

} // namespace simdjson
