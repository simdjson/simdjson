#include "simdjson.h"
#include <initializer_list>

namespace simdjson {

bool implementation::supported_by_runtime_system() const {
  uint32_t required_instruction_sets = this->required_instruction_sets();
  uint32_t supported_instruction_sets = internal::detect_supported_architectures();
  return ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets);
}

namespace internal {

// Static array of known implementations. We're hoping these get baked into the executable
// without requiring a static initializer.

#if SIMDJSON_IMPLEMENTATION_HASWELL
const haswell::implementation haswell_singleton{};
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
const westmere::implementation westmere_singleton{};
#endif // SIMDJSON_IMPLEMENTATION_WESTMERE
#if SIMDJSON_IMPLEMENTATION_ARM64
const arm64::implementation arm64_singleton{};
#endif // SIMDJSON_IMPLEMENTATION_ARM64
#if SIMDJSON_IMPLEMENTATION_FALLBACK
const fallback::implementation fallback_singleton{};
#endif // SIMDJSON_IMPLEMENTATION_FALLBACK

/**
 * @private Detects best supported implementation on first use, and sets it
 */
class detect_best_supported_implementation_on_first_use final : public implementation {
public:
  const std::string &name() const noexcept final { return set_best()->name(); }
  const std::string &description() const noexcept final { return set_best()->description(); }
  uint32_t required_instruction_sets() const noexcept final { return set_best()->required_instruction_sets(); }
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final {
    return set_best()->create_dom_parser_implementation(capacity, max_length, dst);
  }
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final {
    return set_best()->minify(buf, len, dst, dst_len);
  }
  simdjson_warn_unused bool validate_utf8(const char * buf, size_t len) const noexcept final override {
    return set_best()->validate_utf8(buf, len);
  }
  simdjson_really_inline detect_best_supported_implementation_on_first_use() noexcept : implementation("best_supported_detector", "Detects the best supported implementation and sets it", 0) {}
private:
  const implementation *set_best() const noexcept;
};

const detect_best_supported_implementation_on_first_use detect_best_supported_implementation_on_first_use_singleton;

const std::initializer_list<const implementation *> available_implementation_pointers {
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
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t,
    size_t,
    std::unique_ptr<internal::dom_parser_implementation>&
  ) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }
  simdjson_warn_unused error_code minify(const uint8_t *, size_t, uint8_t *, size_t &) const noexcept final override {
    return UNSUPPORTED_ARCHITECTURE;
  }
  simdjson_warn_unused bool validate_utf8(const char *, size_t) const noexcept final override {
    return false; // Just refuse to validate. Given that we have a fallback implementation
    // it seems unlikely that unsupported_implementation will ever be used. If it is used,
    // then it will flag all strings as invalid. The alternative is to return an error_code
    // from which the user has to figure out whether the string is valid UTF-8... which seems
    // like a lot of work just to handle the very unlikely case that we have an unsupported
    // implementation. And, when it does happen (that we have an unsupported implementation),
    // what are the chances that the programmer has a fallback? Given that *we* provide the
    // fallback, it implies that the programmer would need a fallback for our fallback.
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
  uint32_t supported_instruction_sets = internal::detect_supported_architectures();
  for (const implementation *impl : internal::available_implementation_pointers) {
    uint32_t required_instruction_sets = impl->required_instruction_sets();
    if ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets) { return impl; }
  }
  return &unsupported_singleton; // this should never happen?
}

const implementation *detect_best_supported_implementation_on_first_use::set_best() const noexcept {
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  char *force_implementation_name = getenv("SIMDJSON_FORCE_IMPLEMENTATION");
  SIMDJSON_POP_DISABLE_WARNINGS

  if (force_implementation_name) {
    auto force_implementation = available_implementations[force_implementation_name];
    if (force_implementation) {
      return active_implementation = force_implementation;
    } else {
      // Note: abort() and stderr usage within the library is forbidden.
      return active_implementation = &unsupported_singleton;
    }
  }
  return active_implementation = available_implementations.detect_best_supported();
}

} // namespace internal

SIMDJSON_DLLIMPORTEXPORT const internal::available_implementation_list available_implementations{};
SIMDJSON_DLLIMPORTEXPORT internal::atomic_ptr<const implementation> active_implementation{&internal::detect_best_supported_implementation_on_first_use_singleton};

simdjson_warn_unused error_code minify(const char *buf, size_t len, char *dst, size_t &dst_len) noexcept {
  return active_implementation->minify((const uint8_t *)buf, len, (uint8_t *)dst, dst_len);
}
simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) noexcept {
  return active_implementation->validate_utf8(buf, len);
}

const implementation * builtin_implementation() {
  static const implementation * builtin_impl = available_implementations[STRINGIFY(SIMDJSON_BUILTIN_IMPLEMENTATION)];
  return builtin_impl;
}


} // namespace simdjson
