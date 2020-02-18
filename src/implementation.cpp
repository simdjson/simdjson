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
  static const haswell::implementation haswell_singleton{};
  static const westmere::implementation westmere_singleton{};
  const std::vector<const implementation *> implementation::_available_implementations = {
    &haswell_singleton,
    &westmere_singleton,
  };
}

#endif

#ifdef IS_ARM64

#include "arm64/implementation.h"

namespace simdjson {
  static const arm64::implementation arm64_singleton{};
  const std::vector<const implementation *> implementation::_available_implementations = {
    &arm64_singleton
  };
}

#endif

namespace simdjson {

// So we can return UNSUPPORTED_ARCHITECTURE from the parser when there is no support
class unsupported_implementation final : public implementation {
public:
  static const unsupported_implementation singleton();
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

  unsupported_implementation() : implementation("unsupported", "CPU without SIMD instructions", 0) {}
};

static const unsupported_implementation unsupported_singleton{};

const implementation * implementation::from_string(const std::string &arch) noexcept {
  for (const implementation *impl : available_implementations()) {
    if (impl->name() == arch) { return impl; }
  }
  return nullptr;
}

const implementation &implementation::detect() noexcept {
  // They are prelisted in priority order, so we just go down the list
  uint32_t supported_instruction_sets = detect_supported_architectures();
  for (const implementation *impl : available_implementations()) {
    uint32_t required_instruction_sets = impl->required_instruction_sets();
    if ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets) { return *impl; }
  }
  return unsupported_singleton;
}

std::atomic<const implementation *> implementation::_active = &detect();

} // namespace simdjson
