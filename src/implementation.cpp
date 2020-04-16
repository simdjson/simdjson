#include "simdjson.h"
#include "isadetection.h"
#include "simdprune_tables.h"

#include <initializer_list>

// Static array of known implementations. We're hoping these get baked into the executable
// without requiring a static initializer.

#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "haswell/implementation.h"
namespace simdjson { namespace internal { const haswell::implementation haswell_singleton{}; } }
#endif // SIMDJSON_IMPLEMENTATION_HASWELL

#if SIMDJSON_IMPLEMENTATION_WESTMERE
#include "westmere/implementation.h"
namespace simdjson { namespace internal { const westmere::implementation westmere_singleton{}; } }
#endif // SIMDJSON_IMPLEMENTATION_WESTMERE

#if SIMDJSON_IMPLEMENTATION_ARM64
#include "arm64/implementation.h"
namespace simdjson { namespace internal { const arm64::implementation arm64_singleton{}; } }
#endif // SIMDJSON_IMPLEMENTATION_ARM64

#if SIMDJSON_IMPLEMENTATION_FALLBACK
#include "fallback/implementation.h"
namespace simdjson { namespace internal { const fallback::implementation fallback_singleton{}; } }
#endif // SIMDJSON_IMPLEMENTATION_FALLBACK

namespace simdjson {
namespace internal {

/**
 * @private Detects best supported implementation on first use, and sets it
 */
class detect_best_supported_implementation_on_first_use final : public implementation {
public:
  const std::string &name() const noexcept final { return set_best()->name(); }
  const std::string &description() const noexcept final { return set_best()->description(); }
  uint32_t required_instruction_sets() const noexcept final { return set_best()->required_instruction_sets(); }
  WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, dom::parser &parser) const noexcept final {
    return set_best()->parse(buf, len, parser);
  }
  WARN_UNUSED error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final {
    return set_best()->minify(buf, len, dst, dst_len);
  }
  WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, dom::parser &parser, bool streaming) const noexcept final {
    return set_best()->stage1(buf, len, parser, streaming);
  }
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, dom::parser &parser) const noexcept final {
    return set_best()->stage2(buf, len, parser);
  }
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, dom::parser &parser, size_t &next_json) const noexcept final {
    return set_best()->stage2(buf, len, parser, next_json);
  }

  really_inline detect_best_supported_implementation_on_first_use() noexcept : implementation("best_supported_detector", "Detects the best supported implementation and sets it", 0) {}
private:
  const implementation *set_best() const noexcept;
};

const detect_best_supported_implementation_on_first_use detect_best_supported_implementation_on_first_use_singleton;

internal::atomic_ptr<const implementation> active_implementation{&internal::detect_best_supported_implementation_on_first_use_singleton};

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

} // namespace internal

SIMDJSON_DLLIMPORTEXPORT const internal::available_implementation_list available_implementations{};
SIMDJSON_DLLIMPORTEXPORT internal::atomic_ptr<const implementation> active_implementation{&internal::detect_best_supported_implementation_on_first_use_singleton};

} // namespace simdjson
