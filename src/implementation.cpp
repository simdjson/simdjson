#include "simdjson/portability.h"
#include "simdjson/isadetection.h"
#include "simdjson/implementation.h"
#include <vector>

// Static array of known implementations. We're hoping these get baked into the executable
// without requiring a static initializer.

#ifdef IS_X86_64

#include "haswell/implementation.h"
#include "westmere/implementation.h"

namespace simdjson {
  const haswell::implementation haswell_singleton{};
  const westmere::implementation westmere_singleton{};
  const implementation * const available_implementation_pointers [] { &haswell_singleton, &westmere_singleton };
}

#endif

#ifdef IS_ARM64

#include "arm64/implementation.h"

namespace simdjson {
  const arm64::implementation arm64_singleton{};
  const implementation * const available_implementation_pointers[] { &arm64_singleton };
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
  return sizeof(available_implementation_pointers) / sizeof(const implementation *);
}
const implementation *available_implementations::operator[](size_t i) const noexcept {
  return available_implementation_pointers[i];
}
const implementation * const *available_implementations::begin() const noexcept {
  return std::cbegin(available_implementation_pointers);
}
const implementation * const *available_implementations::end() const noexcept {
  return std::cend(available_implementation_pointers);
}
const implementation *available_implementations::detect_best_supported() const noexcept {
  // They are prelisted in priority order, so we just go down the list
  uint32_t supported_instruction_sets = detect_supported_architectures();
  for (const implementation *impl : *this) {
    uint32_t required_instruction_sets = impl->required_instruction_sets();
    if ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets) { return impl; }
  }
  return &unsupported_singleton;
}

} // namespace simdjson
