#ifndef SIMDJSON_SRC_IMPLEMENTATION_CPP
#define SIMDJSON_SRC_IMPLEMENTATION_CPP

#include <base.h>
#include <simdjson/generic/dependencies.h>
#include <simdjson/implementation.h>
#include <internal/isadetection.h>

#include <initializer_list>

namespace simdjson {

bool implementation::supported_by_runtime_system() const {
  uint32_t required_instruction_sets = this->required_instruction_sets();
  uint32_t supported_instruction_sets = internal::detect_supported_architectures();
  return ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets);
}

} // namespace simdjson

#define SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_IMPLEMENTATION_ARM64
#include <simdjson/arm64/implementation.h>
namespace simdjson {
namespace internal {
static const arm64::implementation* get_arm64_singleton() {
  static const arm64::implementation arm64_singleton{};
  return &arm64_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_ARM64

#if SIMDJSON_IMPLEMENTATION_FALLBACK
#include <simdjson/fallback/implementation.h>
namespace simdjson {
namespace internal {
static const fallback::implementation* get_fallback_singleton() {
  static const fallback::implementation fallback_singleton{};
  return &fallback_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_FALLBACK


#if SIMDJSON_IMPLEMENTATION_HASWELL
#include <simdjson/haswell/implementation.h>
namespace simdjson {
namespace internal {
static const haswell::implementation* get_haswell_singleton() {
  static const haswell::implementation haswell_singleton{};
  return &haswell_singleton;
}
} // namespace internal
} // namespace simdjson
#endif

#if SIMDJSON_IMPLEMENTATION_ICELAKE
#include <simdjson/icelake/implementation.h>
namespace simdjson {
namespace internal {
static const icelake::implementation* get_icelake_singleton() {
  static const icelake::implementation icelake_singleton{};
  return &icelake_singleton;
}
} // namespace internal
} // namespace simdjson
#endif

#if SIMDJSON_IMPLEMENTATION_PPC64
#include <simdjson/ppc64/implementation.h>
namespace simdjson {
namespace internal {
static const ppc64::implementation* get_ppc64_singleton() {
  static const ppc64::implementation ppc64_singleton{};
  return &ppc64_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_PPC64

#if SIMDJSON_IMPLEMENTATION_WESTMERE
#include <simdjson/westmere/implementation.h>
namespace simdjson {
namespace internal {
static const simdjson::westmere::implementation* get_westmere_singleton() {
  static const simdjson::westmere::implementation westmere_singleton{};
  return &westmere_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_WESTMERE

#undef SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace internal {

// Static array of known implementations. We're hoping these get baked into the executable
// without requiring a static initializer.

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
  simdjson_inline detect_best_supported_implementation_on_first_use() noexcept : implementation("best_supported_detector", "Detects the best supported implementation and sets it", 0) {}
private:
  const implementation *set_best() const noexcept;
};

static const std::initializer_list<const implementation *>& get_available_implementation_pointers() {
  static const std::initializer_list<const implementation *> available_implementation_pointers {
#if SIMDJSON_IMPLEMENTATION_ICELAKE
    get_icelake_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
    get_haswell_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
    get_westmere_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_ARM64
    get_arm64_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_PPC64
    get_ppc64_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
    get_fallback_singleton(),
#endif
  }; // available_implementation_pointers
  return available_implementation_pointers;
}

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

const unsupported_implementation* get_unsupported_singleton() {
    static const unsupported_implementation unsupported_singleton{};
    return &unsupported_singleton;
}

size_t available_implementation_list::size() const noexcept {
  return internal::get_available_implementation_pointers().size();
}
const implementation * const *available_implementation_list::begin() const noexcept {
  return internal::get_available_implementation_pointers().begin();
}
const implementation * const *available_implementation_list::end() const noexcept {
  return internal::get_available_implementation_pointers().end();
}
const implementation *available_implementation_list::detect_best_supported() const noexcept {
  // They are prelisted in priority order, so we just go down the list
  uint32_t supported_instruction_sets = internal::detect_supported_architectures();
  for (const implementation *impl : internal::get_available_implementation_pointers()) {
    uint32_t required_instruction_sets = impl->required_instruction_sets();
    if ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets) { return impl; }
  }
  return get_unsupported_singleton(); // this should never happen?
}

const implementation *detect_best_supported_implementation_on_first_use::set_best() const noexcept {
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  char *force_implementation_name = getenv("SIMDJSON_FORCE_IMPLEMENTATION");
  SIMDJSON_POP_DISABLE_WARNINGS

  if (force_implementation_name) {
    auto force_implementation = get_available_implementations()[force_implementation_name];
    if (force_implementation) {
      return get_active_implementation() = force_implementation;
    } else {
      // Note: abort() and stderr usage within the library is forbidden.
      return get_active_implementation() = get_unsupported_singleton();
    }
  }
  return get_active_implementation() = get_available_implementations().detect_best_supported();
}

} // namespace internal

SIMDJSON_DLLIMPORTEXPORT const internal::available_implementation_list& get_available_implementations() {
  static const internal::available_implementation_list available_implementations{};
  return available_implementations;
}

SIMDJSON_DLLIMPORTEXPORT internal::atomic_ptr<const implementation>& get_active_implementation() {
    static const internal::detect_best_supported_implementation_on_first_use detect_best_supported_implementation_on_first_use_singleton;
    static internal::atomic_ptr<const implementation> active_implementation{&detect_best_supported_implementation_on_first_use_singleton};
    return active_implementation;
}

simdjson_warn_unused error_code minify(const char *buf, size_t len, char *dst, size_t &dst_len) noexcept {
  return get_active_implementation()->minify(reinterpret_cast<const uint8_t *>(buf), len, reinterpret_cast<uint8_t *>(dst), dst_len);
}
simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) noexcept {
  return get_active_implementation()->validate_utf8(buf, len);
}
const implementation * builtin_implementation() {
  static const implementation * builtin_impl = get_available_implementations()[SIMDJSON_STRINGIFY(SIMDJSON_BUILTIN_IMPLEMENTATION)];
  assert(builtin_impl);
  return builtin_impl;
}

} // namespace simdjson

#endif // SIMDJSON_SRC_IMPLEMENTATION_CPP
